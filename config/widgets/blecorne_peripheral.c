#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/battery.h>
#include <zmk/split/bluetooth/peripheral.h>

#include "util.h"
#include "fonts/pixel_operator_mono.h"
#include "fonts/pixel_operator_mono_large.h"
#include "fonts/icon_font.h"
#include "fonts/status_icon_font.h"
#include "fonts/status_icon_font_wifi.h"
#include "blecorne_peripheral.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ───────────────────────────────────────────────────────────── */

struct peripheral_state {
    uint8_t battery_level;
    bool    connected;
    bool    is_mac;     /* Mac vs Win glyph order, forwarded by central over the sync GATT char */
    bool    is_colemak; /* Qwerty vs Colemak, forwarded the same way - this half has no
                         * local keymap/layer state to derive either flag from */
    uint8_t r_mods;     /* R-mod nibble: bit0=RCtrl, bit1=RShift, bit2=RAlt, bit3=RGUI */
};

static struct peripheral_state widget_state;

/* ── Canvases ────────────────────────────────────────────────────────── *
 * See blecorne_central.c's canvas comment for how these x offsets map to
 * physical display strips. */
static lv_obj_t *canvas_top;    /* x=-44 -> bottom strip (layout name) */
static lv_obj_t *canvas_mid;    /* x=24  -> middle strip (mods)        */
static lv_obj_t *canvas_bot;    /* x=92 (TOP_RIGHT) -> top strip (status) */

static uint8_t cbuf_top[CANVAS_BUF_SIZE];
static uint8_t cbuf_mid[CANVAS_BUF_SIZE];
static uint8_t cbuf_bot[CANVAS_BUF_SIZE];

/* ── Flash state ─────────────────────────────────────────────────────── *
 * Drives the wifi icon's blink while disconnected and the battery-empty
 * blink at <=5% - see blecorne_central.c for the fuller rationale. */

static bool flash_on = true;

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

/* status_icon_font glyph (status row) - see blecorne_central.c. */
static void draw_status_icon(lv_obj_t *canvas, int x, int y, int w, const char *icon,
                             bool active, lv_text_align_t align) {
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, LVGL_FOREGROUND, &status_icon_font);
    dsc.align = align;
    dsc.opa   = active ? LV_OPA_COVER : LV_OPA_40;
    canvas_draw_text(canvas, x, y, w, &dsc, icon);
}

/* Connection icon, drawn from its own smaller font (status_icon_font_wifi,
 * size 16 vs status_icon_font's 18) - it's the widest glyph in the shared
 * status icon font and reads better a size down in this corner. */
static void draw_wifi_icon(lv_obj_t *canvas, int x, int y, int w, bool active,
                           lv_text_align_t align) {
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, LVGL_FOREGROUND, &status_icon_font_wifi);
    dsc.align = align;
    dsc.opa   = active ? LV_OPA_COVER : LV_OPA_40;
    canvas_draw_text(canvas, x, y, w, &dsc, ICON_WIFI_SMALL);
}

/* Modifier cell (border box, invert-on-press) - see blecorne_central.c. */

#define MOD_BOX_W 30
#define MOD_BOX_H 26

static void draw_mod_box(lv_obj_t *canvas, int x, int y, bool active) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius       = 6;
    dsc.border_width = 1;
    dsc.border_color = LVGL_FOREGROUND;
    dsc.border_opa   = LV_OPA_COVER;
    dsc.bg_color     = LVGL_FOREGROUND;
    dsc.bg_opa       = active ? LV_OPA_COVER : LV_OPA_TRANSP;
    canvas_draw_rect(canvas, x, y, MOD_BOX_W, MOD_BOX_H, &dsc);
}

/* icon_w = glyph's real ink width (ICON_*_W), icon_ofs_x = left bearing
 * (ICON_*_OFS_X) - see blecorne_central.c's draw_mod_icon() comment. */
static void draw_mod_icon(lv_obj_t *canvas, int x, int y, const char *icon, int icon_w,
                          int icon_ofs_x, bool active) {
    draw_mod_box(canvas, x, y, active);
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, active ? LVGL_BACKGROUND : LVGL_FOREGROUND, &icon_font);
    dsc.align = LV_TEXT_ALIGN_LEFT;
    int content_x = x + (MOD_BOX_W - icon_w) / 2 - icon_ofs_x;
    canvas_draw_text(canvas, content_x, y + 3, MOD_BOX_W, &dsc, icon);
}

static void draw_mod_text(lv_obj_t *canvas, int x, int y, const char *text, bool active) {
    draw_mod_box(canvas, x, y, active);
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, active ? LVGL_BACKGROUND : LVGL_FOREGROUND, &pixel_operator_mono);
    dsc.align = LV_TEXT_ALIGN_CENTER;
    canvas_draw_text(canvas, x, y + 6, MOD_BOX_W, &dsc, text);
}

/* Discrete battery level -> Font Awesome battery glyph (see blecorne_central.c). */
static const char *battery_icon(uint8_t level) {
    if (level > 75) return ICON_BATTERY_FULL;
    if (level > 50) return ICON_BATTERY_3_4;
    if (level > 25) return ICON_BATTERY_HALF;
    return ICON_BATTERY_QUARTER;
}

/* ── Canvas renderers ────────────────────────────────────────────────── */

/* canvas_bot: status strip. Same two-row layout as blecorne_central.c -
 * connection icon top-left, battery icon top-right, battery % text below
 * it. No BT-profile-equivalent text on this half, so row 2's left side
 * stays blank. The x offsets are hardware-calibrated the same way
 * central's are - see that file's render_status_canvas comment. */
static void render_status_canvas(struct peripheral_state *state) {
    clear_canvas(canvas_bot);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &pixel_operator_mono_large);
    lbl.align = LV_TEXT_ALIGN_RIGHT;

    /* Connection icon is always drawn: full brightness once connected,
     * blinking via flash_on while searching for central - same reasoning
     * as the BT icon in blecorne_central.c (this is a 1-bit display, so
     * "dim" isn't renderable; blinking is what signals "searching"). */
    draw_wifi_icon(canvas_bot, 3, 3, 24, state->connected || flash_on, LV_TEXT_ALIGN_LEFT);

    /* Battery icon is one of five discrete Font Awesome levels (see
     * battery_icon()); at <=5% it blinks (ICON_BATTERY_EMPTY, flash_on
     * only) instead of showing solid - same flash_timer as the connection
     * icon above. */
    if (state->battery_level <= 5) {
        if (flash_on) {
            draw_status_icon(canvas_bot, 36, 0, 24, ICON_BATTERY_EMPTY, true, LV_TEXT_ALIGN_RIGHT);
        }
    } else {
        draw_status_icon(canvas_bot, 36, 0, 24, battery_icon(state->battery_level), true, LV_TEXT_ALIGN_RIGHT);
    }

    char batt_buf[6];
    snprintf(batt_buf, sizeof(batt_buf), "%d%%", state->battery_level);
    canvas_draw_text(canvas_bot, 28, 20, 40, &lbl, batt_buf);

    rotate_canvas(canvas_bot);
}

/* canvas_mid (physical middle strip, 68 px)
 *
 * R-mod nibble bit mapping: bit0=RCtrl, bit1=RShift, bit2=RAlt, bit3=RGUI
 *
 * Same bordered-cell layout and OS-conditional icon-vs-text arrangement as
 * the left half - see blecorne_central.c's render_mod_canvas.
 */
static void render_mod_canvas(struct peripheral_state *state) {
    clear_canvas(canvas_mid);

    /* Section separators - see blecorne_central.c's render_mod_canvas. */
    lv_draw_rect_dsc_t rule_dsc;
    init_rect_dsc(&rule_dsc, LVGL_FOREGROUND);
    canvas_draw_rect(canvas_mid, 0, 0, CANVAS_SIZE, 1, &rule_dsc);
    canvas_draw_rect(canvas_mid, 0, CANVAS_SIZE - 1, CANVAS_SIZE, 1, &rule_dsc);

    bool is_mac = state->is_mac;
    uint8_t r = state->r_mods;

    bool shift_active = !!(r & BIT(1));
    bool ctrl_active  = !!(r & BIT(0));
    bool gui_active   = !!(r & BIT(3));
    bool alt_active   = !!(r & BIT(2));

    int x0 = 4, x1 = 36, y0 = 6, y1 = 38;

    draw_mod_icon(canvas_mid, x0, y0, ICON_SHIFT, ICON_SHIFT_W, ICON_SHIFT_OFS_X, shift_active);
    if (is_mac) {
        draw_mod_icon(canvas_mid, x1, y0, ICON_CTRL, ICON_CTRL_W, ICON_CTRL_OFS_X, ctrl_active);
        draw_mod_icon(canvas_mid, x0, y1, ICON_CMD,  ICON_CMD_W,  ICON_CMD_OFS_X,  gui_active);
        draw_mod_icon(canvas_mid, x1, y1, ICON_OPT,  ICON_OPT_W,  ICON_OPT_OFS_X,  alt_active);
    } else {
        draw_mod_text(canvas_mid, x1, y0, "Ctl", ctrl_active);
        draw_mod_text(canvas_mid, x0, y1, "Win",  gui_active);
        draw_mod_text(canvas_mid, x1, y1, "Alt",  alt_active);
    }

    rotate_canvas(canvas_mid);
}

/* canvas_top (24 px visible, see canvas comment above) - keyboard layout
 * name, forwarded by central over the same sync GATT char as is_mac (this
 * half has no local keymap state to derive it from). Same font and y as
 * central's layer name so the two halves read as a matching pair.
 * "Colmak" rather than "Colemak" - pixel_operator_mono_large is a fixed
 * 10px/char, and "Colemak" (7 chars, 70px) doesn't fit the 68px canvas. */
static void render_layout_canvas(struct peripheral_state *state) {
    clear_canvas(canvas_top);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &pixel_operator_mono_large);
    lbl.align = LV_TEXT_ALIGN_CENTER;
    canvas_draw_text(canvas_top, 0, 5, CANVAS_SIZE, &lbl,
                     state->is_colemak ? "Colmak" : "Qwerty");

    rotate_canvas(canvas_top);
}

/* ── Display work items (run on the ZMK display work queue) ──────────── */

static void status_render_cb(struct k_work *work) {
    render_status_canvas(&widget_state);
}
static K_WORK_DEFINE(status_render_work, status_render_cb);

static void mod_render_cb(struct k_work *work) {
    render_mod_canvas(&widget_state);
}
static K_WORK_DEFINE(mod_render_work, mod_render_cb);

static void layout_render_cb(struct k_work *work) {
    render_layout_canvas(&widget_state);
}
static K_WORK_DEFINE(layout_render_work, layout_render_cb);

static inline void display_submit(struct k_work *work) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), work);
    }
}

/* ── Public modifier update (called by modifier_sync_peripheral) ─────── */

/* Bits 0-3: R-mod nibble. Bit 4: Mac/Win glyph-order flag. Bit 5: Qwerty/
 * Colemak flag. Both forwarded by central, which has no local keymap/layer
 * state of its own to derive either from. */
void blecorne_peripheral_update_mods(uint8_t payload) {
    widget_state.r_mods = payload & 0x0F;
    widget_state.is_mac = !!(payload & BIT(4));
    display_submit(&mod_render_work);

    bool is_colemak = !!(payload & BIT(5));
    if (is_colemak != widget_state.is_colemak) {
        widget_state.is_colemak = is_colemak;
        display_submit(&layout_render_work);
    }
}

/* ── Event listeners ─────────────────────────────────────────────────── */

static int battery_event_cb(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    widget_state.battery_level = ev->state_of_charge;
    display_submit(&status_render_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int split_event_cb(const zmk_event_t *eh) {
    widget_state.connected = zmk_split_bt_peripheral_is_connected();
    display_submit(&status_render_work);
    return ZMK_EV_EVENT_BUBBLE;
}

/* ── Flash work item (submitted by timer, runs on system workq) ───────── */

static void flash_work_cb(struct k_work *work) {
    flash_on = !flash_on;
    display_submit(&status_render_work);
}

ZMK_LISTENER(peri_battery, battery_event_cb);
ZMK_SUBSCRIPTION(peri_battery, zmk_battery_state_changed);

ZMK_LISTENER(peri_split, split_event_cb);
ZMK_SUBSCRIPTION(peri_split, zmk_split_peripheral_status_changed);

/* ── Init ────────────────────────────────────────────────────────────── */

int blecorne_peripheral_widget_init(struct blecorne_peripheral_widget *widget,
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

    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.connected     = zmk_split_bt_peripheral_is_connected();
    widget_state.is_mac        = false;
    widget_state.is_colemak    = false;
    widget_state.r_mods        = 0;

    k_timer_start(&flash_timer, K_MSEC(500), K_MSEC(500));

    render_status_canvas(&widget_state);
    render_mod_canvas(&widget_state);
    render_layout_canvas(&widget_state);

    return 0;
}

lv_obj_t *blecorne_peripheral_widget_obj(struct blecorne_peripheral_widget *widget) {
    return widget->obj;
}
