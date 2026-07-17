#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/hid.h>

#include "util.h"
#include "glyphs.h"
#include "blecorne_central.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ───────────────────────────────────────────────────────────── */

struct central_state {
    uint8_t  battery_level;
    bool     charging;
    uint8_t  active_layer;
    uint32_t mods;          /* HID modifier byte (bitmask) */
};

/* ── Canvases ────────────────────────────────────────────────────────── */

static lv_obj_t *canvas_top;    /* x=92  → physical bottom strip */
static lv_obj_t *canvas_mid;    /* x=24  → physical middle strip */
static lv_obj_t *canvas_bot;    /* x=-44 → physical top strip (24px visible) */

static uint8_t cbuf_top[CANVAS_BUF_SIZE];
static uint8_t cbuf_mid[CANVAS_BUF_SIZE];
static uint8_t cbuf_bot[CANVAS_BUF_SIZE];

/* ── Layer names ─────────────────────────────────────────────────────── */

static const char *layer_names[] = {
    "Qwerty (Win)", "Qwerty (Mac)",
    "Colemak (Win)", "Colemak (Mac)",
    "Num", "Nav", "Sym", "Admin", "Func",
};

#define LAYER_NAME_COUNT ARRAY_SIZE(layer_names)

static const char *get_layer_name(uint8_t idx) {
    if (idx < LAYER_NAME_COUNT) {
        return layer_names[idx];
    }
    return "???";
}

/* ── HID modifier masks ──────────────────────────────────────────────── */

/* Standard HID modifier byte bits */
#define MOD_LCTRL  BIT(0)
#define MOD_LSHIFT BIT(1)
#define MOD_LALT   BIT(2)
#define MOD_LGUI   BIT(3)
#define MOD_RCTRL  BIT(4)
#define MOD_RSHIFT BIT(5)
#define MOD_RALT   BIT(6)
#define MOD_RGUI   BIT(7)

#define MOD_CTRL  (MOD_LCTRL  | MOD_RCTRL)
#define MOD_SHIFT (MOD_LSHIFT | MOD_RSHIFT)
#define MOD_ALT   (MOD_LALT   | MOD_RALT)
#define MOD_GUI   (MOD_LGUI   | MOD_RGUI)

/* ── Draw helpers ────────────────────────────────────────────────────── */

static void clear_canvas(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t r;
    init_rect_dsc(&r, LVGL_BACKGROUND);
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &r);
}

/* ── Canvas renderers ────────────────────────────────────────────────── */

static void render_layer_canvas(struct central_state *state) {
    clear_canvas(canvas_bot);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &lv_font_montserrat_16);

    const char *name = get_layer_name(state->active_layer);
    /* Canvas coords before 270° rotation:
     * After rotation, x in canvas → y on physical display (top→bottom).
     * The "bot" canvas only has 24px visible (x=0..23 in driver space).
     * In canvas space, text should be centred horizontally within 68px.
     * After rotation the canvas x=0 maps to physical y=0 (top).
     * Draw text at canvas y=0 so after rotation it sits at physical left edge. */
    canvas_draw_text(canvas_bot, 0, 0, CANVAS_SIZE, &lbl, name);

    rotate_canvas(canvas_bot);
}

static void render_mod_canvas(struct central_state *state) {
    clear_canvas(canvas_mid);

    bool is_mac = (state->active_layer == 1 || state->active_layer == 3);
    uint32_t mods = state->mods;

    /* Left-side modifier bits only */
    bool shift_active = !!(mods & MOD_LSHIFT);
    bool ctrl_active  = !!(mods & MOD_LCTRL);
    bool gui_active   = !!(mods & MOD_LGUI);
    bool alt_active   = !!(mods & MOD_LALT);

    /* 4 glyphs: ⇧ Shift | ⌃ Ctrl | GUI | Alt
     * 4×14 + 3×3 = 65px → gx=2, step=17 centres in 68px canvas */
    int gx = 2, gy = 27, step = 17;

    draw_glyph(canvas_mid, gx,          gy, &glyph_sft,  shift_active);
    draw_glyph(canvas_mid, gx + step,   gy, &glyph_ctrl, ctrl_active);
    if (is_mac) {
        draw_glyph(canvas_mid, gx + step * 2, gy, &glyph_cmd, gui_active);
        draw_glyph(canvas_mid, gx + step * 3, gy, &glyph_opt, alt_active);
    } else {
        draw_glyph(canvas_mid, gx + step * 2, gy, &glyph_win, gui_active);
        draw_glyph(canvas_mid, gx + step * 3, gy, &glyph_alt, alt_active);
    }

    rotate_canvas(canvas_mid);
}

static void render_battery_canvas(struct central_state *state) {
    clear_canvas(canvas_top);

    /* Draw battery centred in canvas.
     * Battery widget: 24×10 (22 shell + 2 nub).
     * Centre in 68×68: x=(68-24)/2=22, y=(68-10)/2=29. */
    draw_battery(canvas_top, 22, 29, state->battery_level, state->charging);

    /* Level percentage text below battery */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", state->battery_level);
    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &lv_font_montserrat_16);
    canvas_draw_text(canvas_top, 16, 42, 36, &lbl, buf);

    rotate_canvas(canvas_top);
}

/* ── Full redraw ─────────────────────────────────────────────────────── */

static void update_display(struct central_state *state) {
    render_layer_canvas(state);
    render_mod_canvas(state);
    render_battery_canvas(state);
}

/* ── ZMK widget listener state + callbacks ───────────────────────────── */

static struct central_state widget_state;

static void set_battery_state(struct central_state *state,
                               const struct zmk_battery_state_changed *ev) {
    state->battery_level = ev->state_of_charge;
    state->charging = (zmk_usb_is_powered());
}

static void set_layer_state(struct central_state *state,
                             const struct zmk_layer_state_changed *ev) {
    state->active_layer = zmk_keymap_highest_layer_active();
}

/* Modifier state via keycode events — check HID modifier state after each key event */
static void set_mod_state(struct central_state *state) {
    zmk_mod_flags_t zmk_mods = zmk_hid_get_explicit_mods();
    state->mods = zmk_mods;
}

/* ── Event listeners ─────────────────────────────────────────────────── */

static int battery_event_cb(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    set_battery_state(&widget_state, ev);
    ZMK_DISPLAY_LOCK();
    render_battery_canvas(&widget_state);
    ZMK_DISPLAY_UNLOCK();
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_event_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    set_layer_state(&widget_state, ev);
    ZMK_DISPLAY_LOCK();
    render_layer_canvas(&widget_state);
    render_mod_canvas(&widget_state);
    ZMK_DISPLAY_UNLOCK();
    return ZMK_EV_EVENT_BUBBLE;
}

static int keycode_event_cb(const zmk_event_t *eh) {
    set_mod_state(&widget_state);
    ZMK_DISPLAY_LOCK();
    render_mod_canvas(&widget_state);
    ZMK_DISPLAY_UNLOCK();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_battery, battery_event_cb);
ZMK_SUBSCRIPTION(central_battery, zmk_battery_state_changed);

ZMK_LISTENER(central_layer, layer_event_cb);
ZMK_SUBSCRIPTION(central_layer, zmk_layer_state_changed);

ZMK_LISTENER(central_keycode, keycode_event_cb);
ZMK_SUBSCRIPTION(central_keycode, zmk_keycode_state_changed);

/* ── Init ────────────────────────────────────────────────────────────── */

int blecorne_central_widget_init(struct blecorne_central_widget *widget,
                                 lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(widget->obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);

    /* Three 68×68 canvases mirroring nice_view's layout */
    canvas_top = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_top, cbuf_top, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_top, LV_ALIGN_TOP_RIGHT, 0, 0);

    canvas_mid = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_mid, cbuf_mid, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_mid, LV_ALIGN_TOP_LEFT, 24, 0);

    canvas_bot = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_bot, cbuf_bot, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_bot, LV_ALIGN_TOP_LEFT, -44, 0);

    /* Seed initial state */
    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.charging      = zmk_usb_is_powered();
    widget_state.active_layer  = zmk_keymap_highest_layer_active();
    widget_state.mods          = zmk_hid_get_explicit_mods();

    update_display(&widget_state);

    return 0;
}

lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget) {
    return widget->obj;
}
