#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "util.h"
#include "fonts/pixel_operator_mono_large.h"
#include "fonts/pixel_operator_mono_xl.h"
#include "fonts/status_icon_font.h"
#include "blecorne_central.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ───────────────────────────────────────────────────────────── */

struct central_state {
    uint8_t  battery_level;
    bool     charging;
    uint8_t  active_layer;
    bool     ble_connected;
    uint8_t  ble_profile;
};

static struct central_state widget_state;

/* ── Canvases ────────────────────────────────────────────────────────── *
 * Each canvas is a 68x68 LVGL object, drawn to and then rotated 270°
 * before the driver composites all three into the nice!view's 68x160
 * physical buffer. The x offset below is what determines which physical
 * strip a canvas lands in - see CLAUDE.md's Canvas layout section for the
 * full offset/strip mapping. canvas_top lands in the strip that only
 * exposes its first 24 rows; canvas_mid and canvas_bot get their full 68.
 */
static lv_obj_t *canvas_top;    /* x=-44 -> bottom strip, 24px visible (blank)      */
static lv_obj_t *canvas_mid;    /* x=24  -> middle strip (layer name)               */
static lv_obj_t *canvas_bot;    /* x=92 (TOP_RIGHT) -> top strip (status)           */

static uint8_t cbuf_top[CANVAS_BUF_SIZE];
static uint8_t cbuf_mid[CANVAS_BUF_SIZE];
static uint8_t cbuf_bot[CANVAS_BUF_SIZE];

/* ── Layer names ─────────────────────────────────────────────────────── */

/* Layers 0-2 (Qwerty Win/Win-profile/Mac) all show "Base" - the display
 * doesn't distinguish the Win vs Mac profile. */
static const char *layer_names[] = {
    "Base", "Base", "Base",
    /* Order must match blecorne.keymap's layer indices - Admin (7) has to
     * stay numerically above Func (6), see the conditional_layers comment
     * there for why. */
    "Num", "Nav", "Sym", "Func", "Admin",
};

#define LAYER_NAME_COUNT ARRAY_SIZE(layer_names)

static const char *get_layer_name(uint8_t idx) {
    return (idx < LAYER_NAME_COUNT) ? layer_names[idx] : "???";
}

/* ── Flash state ─────────────────────────────────────────────────────── *
 * One 500ms heartbeat drives every blink on this half: the BT icon while
 * searching, and the battery-empty icon at <=5%. Runs continuously
 * (rather than only while BT is disconnected) since battery-empty needs
 * it regardless of BT state. */

static bool bt_connected = false;
static bool flash_on     = true;

static void flash_work_cb(struct k_work *work);
static K_WORK_DEFINE(flash_work, flash_work_cb);

static void flash_timer_cb(struct k_timer *t) {
    k_work_submit(&flash_work);
}
static K_TIMER_DEFINE(flash_timer, flash_timer_cb, NULL);

/* ── Draw helpers ────────────────────────────────────────────────────── */

static void clear_canvas(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t r;
    init_rect_dsc(&r, LVGL_BACKGROUND);
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &r);
}

/* status_icon_font glyph (status row). `align` picks LEFT for the BT icon
 * (hugs the left edge) or RIGHT for the battery/bolt icon (hugs the right
 * edge, stacking above the right-aligned % text below it). `active` is
 * full brightness vs LV_OPA_40 - on this 1-bit display LV_OPA_40 renders
 * as fully invisible rather than dim, so callers that want a "blinking"
 * rather than "dim" look need to alternate `active` with `flash_on`
 * themselves (see the BT icon below). */
static void draw_status_icon(lv_obj_t *canvas, int x, int y, int w, const char *icon,
                             bool active, lv_text_align_t align) {
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, LVGL_FOREGROUND, &status_icon_font);
    dsc.align = align;
    dsc.opa   = active ? LV_OPA_COVER : LV_OPA_40;
    canvas_draw_text(canvas, x, y, w, &dsc, icon);
}

/* Discrete battery level -> Font Awesome battery glyph. <=5% is handled by
 * the caller separately (blinks via ICON_BATTERY_EMPTY instead of showing
 * solid). */
static const char *battery_icon(uint8_t level) {
    if (level > 75) return ICON_BATTERY_FULL;
    if (level > 50) return ICON_BATTERY_3_4;
    if (level > 25) return ICON_BATTERY_HALF;
    return ICON_BATTERY_QUARTER;
}

/* Friendly 2-character names for BT profile slots that are always paired
 * to the same device, so the status row can show e.g. "WL" instead of
 * "B0". Profiles without an entry here fall back to "B<n>". Update this
 * table (not the keymap) if a profile slot gets re-paired to a different
 * device. */
static const char *profile_alias(uint8_t profile) {
    switch (profile) {
        case 0: return "WL"; /* Work laptop */
        case 1: return "MB"; /* MacBook */
        case 2: return "iP"; /* iPad */
        default: return NULL;
    }
}

/* ── Canvas renderers ────────────────────────────────────────────────── */

/* canvas_bot: status strip. Row 1 (y=0) is icons - BT top-left, battery/
 * bolt top-right. Row 2 (y=20) is text in the same left/right arrangement,
 * `pixel_operator_mono_large`. The exact x offsets below are calibrated
 * against a real nice!view module, not derived from a formula - the icon
 * fonts' rasterized ink doesn't line up edge-to-edge with the canvas
 * boundary the way the math would suggest, so don't "simplify" these back
 * to round numbers without re-checking on hardware. */
static void render_status_canvas(struct central_state *state) {
    clear_canvas(canvas_bot);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &pixel_operator_mono_large);

    /* BT icon is always drawn (never disappears): full brightness once
     * connected, blinking via flash_on while still searching. */
    draw_status_icon(canvas_bot, 0, 0, 24, ICON_BT, bt_connected || flash_on, LV_TEXT_ALIGN_LEFT);

    if (state->charging) {
        draw_status_icon(canvas_bot, 36, 0, 24, ICON_BOLT, true, LV_TEXT_ALIGN_RIGHT);
    } else if (state->battery_level <= 5) {
        if (flash_on) {
            draw_status_icon(canvas_bot, 36, 0, 24, ICON_BATTERY_EMPTY, true, LV_TEXT_ALIGN_RIGHT);
        }
    } else {
        draw_status_icon(canvas_bot, 36, 0, 24, battery_icon(state->battery_level), true, LV_TEXT_ALIGN_RIGHT);
    }

    /* pixel_operator_mono_large is a fixed 10px/char, so field widths are
     * sized off exact character counts: "100%" (4 chars) is 40px, which is
     * why the profile label is abbreviated to "B<n>"/an alias (max 2
     * chars) rather than "BT <n>" - the full label doesn't fit next to a
     * 4-digit battery percentage in 68px. */
    lbl.align = LV_TEXT_ALIGN_RIGHT;
    char batt_buf[6];
    snprintf(batt_buf, sizeof(batt_buf), "%d%%", state->battery_level);
    canvas_draw_text(canvas_bot, 28, 20, 40, &lbl, batt_buf);

    if (bt_connected) {
        lbl.align = LV_TEXT_ALIGN_LEFT;
        char profile_buf[4];
        const char *alias = profile_alias(state->ble_profile);
        if (alias) {
            snprintf(profile_buf, sizeof(profile_buf), "%s", alias);
        } else {
            snprintf(profile_buf, sizeof(profile_buf), "B%d", state->ble_profile);
        }
        canvas_draw_text(canvas_bot, 0, 20, 22, &lbl, profile_buf);
    }

    rotate_canvas(canvas_bot);
}

/* canvas_mid: layer name, centered in the full 68px canvas, plus 1px rules
 * at y=0 and y=67 as section separators. This canvas has no hardware
 * truncation, so the rules land exactly on the physical boundaries with the
 * status strip above and the blank strip below.
 *
 * y=24: pixel_operator_mono_xl's line_height is 20 and every layer name is
 * cap-height only (14px, no descenders), so the ink spans rows 27-40 -
 * centered in 68. */
static void render_layer_canvas(struct central_state *state) {
    clear_canvas(canvas_mid);

    lv_draw_rect_dsc_t rule_dsc;
    init_rect_dsc(&rule_dsc, LVGL_FOREGROUND);
    canvas_draw_rect(canvas_mid, 0, 0, CANVAS_SIZE, 1, &rule_dsc);
    canvas_draw_rect(canvas_mid, 0, CANVAS_SIZE - 1, CANVAS_SIZE, 1, &rule_dsc);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &pixel_operator_mono_xl);
    lbl.align = LV_TEXT_ALIGN_CENTER;
    canvas_draw_text(canvas_mid, 0, 24, CANVAS_SIZE, &lbl,
                     get_layer_name(state->active_layer));

    rotate_canvas(canvas_mid);
}

/* canvas_top (24px visible) - unused, cleared once at init. */
static void render_blank_canvas(void) {
    clear_canvas(canvas_top);
    rotate_canvas(canvas_top);
}

/* ── Display work items (run on the ZMK display work queue) ──────────── */

static void status_render_cb(struct k_work *work) {
    render_status_canvas(&widget_state);
}
static K_WORK_DEFINE(status_render_work, status_render_cb);

static void layer_render_cb(struct k_work *work) {
    render_layer_canvas(&widget_state);
}
static K_WORK_DEFINE(layer_render_work, layer_render_cb);

static inline void display_submit(struct k_work *work) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), work);
    }
}

/* ── Flash work item (submitted by timer, runs on system workq) ───────── */

/* Only redraw while something actually blinks - otherwise the toggle changes
 * nothing visible and a redraw just costs CPU, SPI and battery twice a
 * second. Events that start or stop a blink redraw on their own. */
static bool needs_blink(void) {
    return !bt_connected || (!widget_state.charging && widget_state.battery_level <= 5);
}

static void flash_work_cb(struct k_work *work) {
    flash_on = !flash_on;
    if (needs_blink()) {
        display_submit(&status_render_work);
    }
}

/* ── Event listeners ─────────────────────────────────────────────────── */

static int battery_event_cb(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    widget_state.battery_level = ev->state_of_charge;
    widget_state.charging      = zmk_usb_is_powered();
    display_submit(&status_render_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_event_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    widget_state.active_layer = zmk_keymap_highest_layer_active();
    display_submit(&layer_render_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int ble_event_cb(const zmk_event_t *eh) {
    bt_connected               = zmk_ble_active_profile_is_connected();
    widget_state.ble_profile   = zmk_ble_active_profile_index();
    widget_state.ble_connected = bt_connected;
    display_submit(&status_render_work);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_battery, battery_event_cb);
ZMK_SUBSCRIPTION(central_battery, zmk_battery_state_changed);

ZMK_LISTENER(central_layer, layer_event_cb);
ZMK_SUBSCRIPTION(central_layer, zmk_layer_state_changed);

ZMK_LISTENER(central_ble, ble_event_cb);
ZMK_SUBSCRIPTION(central_ble, zmk_ble_active_profile_changed);

/* ── Init ────────────────────────────────────────────────────────────── */

int blecorne_central_widget_init(struct blecorne_central_widget *widget,
                                 lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(widget->obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);

    canvas_top = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_top, cbuf_top, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_top, LV_ALIGN_TOP_LEFT, -44, 0);

    canvas_mid = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_mid, cbuf_mid, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_mid, LV_ALIGN_TOP_LEFT, 24, 0);

    canvas_bot = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_bot, cbuf_bot, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_bot, LV_ALIGN_TOP_RIGHT, 0, 0);

    bt_connected = zmk_ble_active_profile_is_connected();

    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.charging      = zmk_usb_is_powered();
    widget_state.active_layer  = zmk_keymap_highest_layer_active();
    widget_state.ble_connected = bt_connected;
    widget_state.ble_profile   = zmk_ble_active_profile_index();

    k_timer_start(&flash_timer, K_MSEC(500), K_MSEC(500));

    render_status_canvas(&widget_state);
    render_layer_canvas(&widget_state);
    render_blank_canvas();

    return 0;
}

lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget) {
    return widget->obj;
}
