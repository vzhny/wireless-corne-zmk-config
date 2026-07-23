#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/hid.h>
#include <zmk/ble.h>

#include "util.h"
#include "fonts/pixel_operator_mono.h"
#include "fonts/pixel_operator_mono_large.h"
#include "fonts/icon_font.h"
#include "fonts/status_icon_font.h"
#include "blecorne_central.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ───────────────────────────────────────────────────────────── */

struct central_state {
    uint8_t  battery_level;
    bool     charging;
    uint8_t  active_layer;
    uint32_t mods;
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
static lv_obj_t *canvas_top;    /* x=-44 -> bottom strip, 24px visible (layer name) */
static lv_obj_t *canvas_mid;    /* x=24  -> middle strip (modifiers)                */
static lv_obj_t *canvas_bot;    /* x=92 (TOP_RIGHT) -> top strip (status)           */

static uint8_t cbuf_top[CANVAS_BUF_SIZE];
static uint8_t cbuf_mid[CANVAS_BUF_SIZE];
static uint8_t cbuf_bot[CANVAS_BUF_SIZE];

/* ── Layer names ─────────────────────────────────────────────────────── */

/* Layers 0-3 (Qwerty/Colemak x Win/Mac) all show "Base" - the modifier
 * row's icons-vs-text already shows Win vs Mac (see render_mod_canvas),
 * and the peripheral's layout row shows Qwerty vs Colemak, so this name
 * doesn't need to carry either distinction, and there isn't room for a
 * full "Colemak (Win)"-style name at this font size anyway. */
static const char *layer_names[] = {
    "Base", "Base",
    "Base", "Base",
    /* Order must match blecorne.keymap's layer indices - Admin (8) has to
     * stay numerically above Func (7), see the conditional_layers comment
     * there for why. */
    "Num", "Nav", "Sym", "Func", "Admin",
};

#define LAYER_NAME_COUNT ARRAY_SIZE(layer_names)

static const char *get_layer_name(uint8_t idx) {
    return (idx < LAYER_NAME_COUNT) ? layer_names[idx] : "???";
}

/* ── HID modifier masks ──────────────────────────────────────────────── *
 * Standard HID modifier byte layout - bits 0-3 are the left-hand mods this
 * half's own display reads, bits 4-7 are the right-hand mods forwarded to
 * the peripheral (see modifier_sync_central.c). */

#define MOD_LCTRL  BIT(0)
#define MOD_LSHIFT BIT(1)
#define MOD_LALT   BIT(2)
#define MOD_LGUI   BIT(3)
#define MOD_RCTRL  BIT(4)
#define MOD_RSHIFT BIT(5)
#define MOD_RALT   BIT(6)
#define MOD_RGUI   BIT(7)

/* Display-only approximation of "currently held" for the mod keys, since
 * the real HID mod state can lag well behind the physical press for any
 * hold-tap key - see the shadow-tracking section below for how this gets
 * set. Combined with the real mods (state->mods | shadow_mods) wherever
 * mods are shown or forwarded to the peripheral - never fed back into
 * actual HID output. */
static uint8_t shadow_mods = 0;

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

/* ── Modifier cell ─────────────────────────────────────────────────────
 * Each modifier key is a MOD_BOX_W x MOD_BOX_H box with a 1px rounded
 * border, always visible. Pressed ("active") fills the box with the
 * foreground color and inverts the icon/text color inside it - a "lit
 * key" look. */

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

/* icon_font's glyphs don't share a common visual center: LV_TEXT_ALIGN_CENTER
 * centers by the font's shared advance width, not each glyph's actual ink,
 * so it visibly mis-centers these. `icon_w`/`icon_ofs_x` (from icon_font.h's
 * ICON_*_W/ICON_*_OFS_X) are the real per-glyph ink width and left bearing,
 * used to compute the correct centered x by hand instead. */
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
        draw_status_icon(canvas_bot, 40, 0, 24, ICON_BOLT, true, LV_TEXT_ALIGN_RIGHT);
    } else if (state->battery_level <= 5) {
        if (flash_on) {
            draw_status_icon(canvas_bot, 40, 0, 24, ICON_BATTERY_EMPTY, true, LV_TEXT_ALIGN_RIGHT);
        }
    } else {
        draw_status_icon(canvas_bot, 40, 0, 24, battery_icon(state->battery_level), true, LV_TEXT_ALIGN_RIGHT);
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

/* canvas_mid: modifier row. Two rows of bordered cells - Shift+Ctrl at
 * y=6, GUI+Alt at y=38, columns at x=4/x=36.
 *
 * Mac shows all four as icons (real keycap symbols). Windows shows Shift
 * as an icon but Ctrl/GUI/Alt as text ("Ctl"/"Win"/"Alt") since that's
 * what's actually printed on a Windows keycap - this asymmetry is
 * intentional, not a gap to fill in later. "Ctl" (not "Ctrl") because
 * "Ctrl" wraps to a second line at this box/font size. */
static void render_mod_canvas(struct central_state *state) {
    clear_canvas(canvas_mid);

    /* This canvas uses its full 68px budget with no truncation, so a rule
     * at y=0 and y=67 lands exactly on the physical boundary with the
     * status strip above and layer strip below. */
    lv_draw_rect_dsc_t rule_dsc;
    init_rect_dsc(&rule_dsc, LVGL_FOREGROUND);
    canvas_draw_rect(canvas_mid, 0, 0, CANVAS_SIZE, 1, &rule_dsc);
    canvas_draw_rect(canvas_mid, 0, CANVAS_SIZE - 1, CANVAS_SIZE, 1, &rule_dsc);

    bool is_mac = (state->active_layer == 1 || state->active_layer == 3);
    uint32_t mods = state->mods | shadow_mods;

    bool shift_active = !!(mods & MOD_LSHIFT);
    bool ctrl_active  = !!(mods & MOD_LCTRL);
    bool gui_active   = !!(mods & MOD_LGUI);
    bool alt_active   = !!(mods & MOD_LALT);

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

/* canvas_top: layer name, centered. y=5 rather than vertically centered in
 * the full 68px canvas, since only rows 0-23 of this canvas actually reach
 * the screen (see the canvas_top comment above). */
static void render_layer_canvas(struct central_state *state) {
    clear_canvas(canvas_top);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &pixel_operator_mono_large);
    lbl.align = LV_TEXT_ALIGN_CENTER;
    canvas_draw_text(canvas_top, 0, 5, CANVAS_SIZE, &lbl,
                     get_layer_name(state->active_layer));

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

static void layer_mod_render_cb(struct k_work *work) {
    render_layer_canvas(&widget_state);
    render_mod_canvas(&widget_state); /* layer change can flip Mac vs Win glyph order */
}
static K_WORK_DEFINE(layer_mod_render_work, layer_mod_render_cb);

static inline void display_submit(struct k_work *work) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), work);
    }
}

/* ── Flash work item (submitted by timer, runs on system workq) ───────── */

static void flash_work_cb(struct k_work *work) {
    flash_on = !flash_on;
    display_submit(&status_render_work);
}

/* ── Modifier shadow-tracking (display-only) ──────────────────────────── *
 *
 * Real HID mod state only updates once ZMK's hold-tap logic actually
 * decides tap vs hold, which for "balanced" flavor can lag well behind the
 * physical press (see CLAUDE.md's Homerow mods section for why). This
 * watches raw key presses on the known modifier positions directly and
 * manually lights up the display after that position's own
 * tapping-term-ms, without waiting for the real decision - approximating
 * "hold long enough and it's obviously a hold" the same way a person
 * would judge it by eye.
 *
 * This is a deliberate approximation, not a substitute for the real HID
 * state: it only ever adds bits to the display (shadow_mods), never to
 * actual HID output, and it can disagree with the real decision - e.g. a
 * quick type-through that resolves HOLD via an interrupt release before
 * this timer fires won't light up here, and a position that's also part
 * of a combo (see blecorne.keymap) can show a slightly late shadow light
 * if that combo doesn't end up firing. Both are accepted trade-offs for
 * getting real-time feedback out of a fundamentally non-instant mechanism.
 *
 * Position numbers and which logical mod each one is come straight from
 * blecorne.keymap's homerow/thumb bindings - if those ever move, update
 * this table too. */

struct shadow_mod_slot {
    uint32_t position;
    uint32_t tapping_term_ms;
    bool     swaps_with_mac; /* true for the two Ctrl/Gui-swap positions per hand */
    uint8_t  bit_win;        /* bit when !swaps_with_mac, or when swaps_with_mac && !is_mac */
    uint8_t  bit_mac;        /* bit when swaps_with_mac && is_mac (unused otherwise) */
    bool     held;
    bool     fired;
    uint8_t  applied_bit;
    struct k_work_delayable work;
};

static void shadow_slot_timeout(struct k_work *work);

static struct shadow_mod_slot shadow_slots[] = {
    /* Left homerow: A/S/D (Ctrl/Gui swap with Mac layers, Alt fixed) */
    { .position = 13, .tapping_term_ms = 280, .swaps_with_mac = true,  .bit_win = MOD_LCTRL, .bit_mac = MOD_LGUI },
    { .position = 14, .tapping_term_ms = 280, .swaps_with_mac = true,  .bit_win = MOD_LGUI,  .bit_mac = MOD_LCTRL },
    { .position = 15, .tapping_term_ms = 280, .swaps_with_mac = false, .bit_win = MOD_LALT },
    /* Right homerow: K/L/; (mirrors the left hand's swap) */
    { .position = 20, .tapping_term_ms = 280, .swaps_with_mac = false, .bit_win = MOD_RALT },
    { .position = 21, .tapping_term_ms = 280, .swaps_with_mac = true,  .bit_win = MOD_RGUI,  .bit_mac = MOD_RCTRL },
    { .position = 22, .tapping_term_ms = 280, .swaps_with_mac = true,  .bit_win = MOD_RCTRL, .bit_mac = MOD_RGUI },
    /* Thumb shift keys - never swap with Mac/Win */
    { .position = 38, .tapping_term_ms = 225, .swaps_with_mac = false, .bit_win = MOD_LSHIFT },
    { .position = 39, .tapping_term_ms = 225, .swaps_with_mac = false, .bit_win = MOD_RSHIFT },
};
#define SHADOW_SLOT_COUNT ARRAY_SIZE(shadow_slots)

static void shadow_slot_timeout(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct shadow_mod_slot *slot = CONTAINER_OF(dwork, struct shadow_mod_slot, work);
    if (!slot->held) {
        return; /* released before the timer fired - a tap, not a hold */
    }
    bool is_mac = (widget_state.active_layer == 1 || widget_state.active_layer == 3);
    slot->applied_bit = (slot->swaps_with_mac && is_mac) ? slot->bit_mac : slot->bit_win;
    slot->fired = true;
    shadow_mods |= slot->applied_bit;
    display_submit(&mod_render_work);
}

static int position_event_cb(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (size_t i = 0; i < SHADOW_SLOT_COUNT; i++) {
        struct shadow_mod_slot *slot = &shadow_slots[i];
        if (slot->position != ev->position) {
            continue;
        }
        if (ev->state) {
            slot->held  = true;
            slot->fired = false;
            k_work_schedule(&slot->work, K_MSEC(slot->tapping_term_ms));
        } else {
            slot->held = false;
            k_work_cancel_delayable(&slot->work);
            if (slot->fired) {
                slot->fired = false;
                shadow_mods &= ~slot->applied_bit;
                display_submit(&mod_render_work);
            }
        }
        break;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_position, position_event_cb);
ZMK_SUBSCRIPTION(central_position, zmk_position_state_changed);

/* Combined real+shadow mods, 8-bit HID shape (bits 0-3 left, 4-7 right) -
 * used by modifier_sync_central.c to forward the right-hand nibble (with
 * its shadow bits) to the peripheral, the same way it already forwards
 * the real R-mod nibble. */
uint8_t blecorne_central_get_display_mods(void) {
    return (uint8_t)((widget_state.mods | shadow_mods) & 0xFF);
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
    display_submit(&layer_mod_render_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int keycode_event_cb(const zmk_event_t *eh) {
    widget_state.mods = zmk_hid_get_explicit_mods();
    display_submit(&mod_render_work);
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

ZMK_LISTENER(central_keycode, keycode_event_cb);
ZMK_SUBSCRIPTION(central_keycode, zmk_keycode_state_changed);

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
    widget_state.mods          = zmk_hid_get_explicit_mods();
    widget_state.ble_connected = bt_connected;
    widget_state.ble_profile   = zmk_ble_active_profile_index();

    k_timer_start(&flash_timer, K_MSEC(500), K_MSEC(500));

    for (size_t i = 0; i < SHADOW_SLOT_COUNT; i++) {
        k_work_init_delayable(&shadow_slots[i].work, shadow_slot_timeout);
    }

    render_status_canvas(&widget_state);
    render_mod_canvas(&widget_state);
    render_layer_canvas(&widget_state);

    return 0;
}

lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget) {
    return widget->obj;
}
