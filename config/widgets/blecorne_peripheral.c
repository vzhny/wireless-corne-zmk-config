#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/battery.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/keymap.h>

#include "util.h"
#include "glyphs.h"
#include "blecorne_peripheral.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ───────────────────────────────────────────────────────────── */

struct peripheral_state {
    uint8_t battery_level;
    bool    connected;
    uint8_t active_layer;
    uint8_t r_mods; /* R-mod nibble: bit0=RCtrl, bit1=RShift, bit2=RAlt, bit3=RGUI */
};

static struct peripheral_state widget_state;

/* ── Canvases ────────────────────────────────────────────────────────── */

static lv_obj_t *canvas_top;    /* x=92  → physical bottom strip (blank)   */
static lv_obj_t *canvas_mid;    /* x=24  → physical middle strip (mods)    */
static lv_obj_t *canvas_bot;    /* x=-44 → physical top strip   (status)   */

static uint8_t cbuf_top[CANVAS_BUF_SIZE];
static uint8_t cbuf_mid[CANVAS_BUF_SIZE];
static uint8_t cbuf_bot[CANVAS_BUF_SIZE];

/* ── Draw helpers ────────────────────────────────────────────────────── */

static void clear_canvas(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t r;
    init_rect_dsc(&r, LVGL_BACKGROUND);
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &r);
}

/* ── Canvas renderers ────────────────────────────────────────────────── */

/* canvas_bot (physical top strip, 24 px visible = canvas rows 0..23)
 *
 * Layout (canvas space):
 *   y=0..11  : "LINK"/"----" at x=0, battery % at x=40  (Montserrat 12)
 *   y=13..23 : battery shell at x=23, centred in 68 px  (22×10)
 */
static void render_status_canvas(struct peripheral_state *state) {
    clear_canvas(canvas_bot);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &lv_font_montserrat_12);

    /* Connection status — left of top line */
    const char *conn_str = state->connected ? "LINK" : "----";
    canvas_draw_text(canvas_bot, 0, 0, 38, &lbl, conn_str);

    /* Battery percentage — right of top line */
    char batt_buf[6];
    snprintf(batt_buf, sizeof(batt_buf), "%d%%", state->battery_level);
    canvas_draw_text(canvas_bot, 40, 0, 28, &lbl, batt_buf);

    /* Battery icon centred at y=13: (68-24)/2 = 22 → x=23 */
    draw_battery(canvas_bot, 23, 13, state->battery_level, false);

    rotate_canvas(canvas_bot);
}

/* canvas_mid (physical middle strip, 68 px)
 *
 * R-mod nibble bit mapping: bit0=RCtrl, bit1=RShift, bit2=RAlt, bit3=RGUI
 *
 * Glyph order mirrors left side (right-to-left):
 *   Win: Alt  ⊞  ⌃  ⇧
 *   Mac: ⌥  ⌃  ⌘  ⇧
 */
static void render_mod_canvas(struct peripheral_state *state) {
    clear_canvas(canvas_mid);

    bool is_mac = (state->active_layer == 1 || state->active_layer == 3);
    uint8_t r = state->r_mods;

    bool shift_active = !!(r & BIT(1));
    bool ctrl_active  = !!(r & BIT(0));
    bool gui_active   = !!(r & BIT(3));
    bool alt_active   = !!(r & BIT(2));

    int gx = 2, gy = 27, step = 17;

    if (is_mac) {
        /* Mac: ⌥ ⌃ ⌘ ⇧ */
        draw_glyph(canvas_mid, gx,          gy, &glyph_opt,  alt_active);
        draw_glyph(canvas_mid, gx + step,   gy, &glyph_ctrl, ctrl_active);
        draw_glyph(canvas_mid, gx + step*2, gy, &glyph_cmd,  gui_active);
        draw_glyph(canvas_mid, gx + step*3, gy, &glyph_sft,  shift_active);
    } else {
        /* Win: Alt ⊞ ⌃ ⇧ */
        draw_glyph(canvas_mid, gx,          gy, &glyph_alt,  alt_active);
        draw_glyph(canvas_mid, gx + step,   gy, &glyph_win,  gui_active);
        draw_glyph(canvas_mid, gx + step*2, gy, &glyph_ctrl, ctrl_active);
        draw_glyph(canvas_mid, gx + step*3, gy, &glyph_sft,  shift_active);
    }

    rotate_canvas(canvas_mid);
}

/* canvas_top (physical bottom strip, 68 px) — intentionally blank */
static void render_blank_canvas(void) {
    clear_canvas(canvas_top);
    rotate_canvas(canvas_top);
}

/* ── Public modifier update (called by modifier_sync_peripheral) ─────── */

void blecorne_peripheral_update_mods(uint8_t r_mods) {
    widget_state.r_mods = r_mods;
    ZMK_DISPLAY_LOCK();
    render_mod_canvas(&widget_state);
    ZMK_DISPLAY_UNLOCK();
}

/* ── Event listeners ─────────────────────────────────────────────────── */

static int battery_event_cb(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    widget_state.battery_level = ev->state_of_charge;
    ZMK_DISPLAY_LOCK();
    render_status_canvas(&widget_state);
    ZMK_DISPLAY_UNLOCK();
    return ZMK_EV_EVENT_BUBBLE;
}

static int split_event_cb(const zmk_event_t *eh) {
    widget_state.connected = zmk_split_bt_peripheral_is_connected();
    ZMK_DISPLAY_LOCK();
    render_status_canvas(&widget_state);
    ZMK_DISPLAY_UNLOCK();
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_event_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    widget_state.active_layer = zmk_keymap_highest_layer_active();
    ZMK_DISPLAY_LOCK();
    render_mod_canvas(&widget_state); /* layer affects Mac vs Win glyph order */
    ZMK_DISPLAY_UNLOCK();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(peri_battery, battery_event_cb);
ZMK_SUBSCRIPTION(peri_battery, zmk_battery_state_changed);

ZMK_LISTENER(peri_split, split_event_cb);
ZMK_SUBSCRIPTION(peri_split, zmk_split_peripheral_status_changed);

ZMK_LISTENER(peri_layer, layer_event_cb);
ZMK_SUBSCRIPTION(peri_layer, zmk_layer_state_changed);

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
    lv_obj_align(canvas_top, LV_ALIGN_TOP_RIGHT, 0, 0);

    canvas_mid = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_mid, cbuf_mid, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_mid, LV_ALIGN_TOP_LEFT, 24, 0);

    canvas_bot = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_bot, cbuf_bot, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_bot, LV_ALIGN_TOP_LEFT, -44, 0);

    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.connected     = zmk_split_bt_peripheral_is_connected();
    widget_state.active_layer  = zmk_keymap_highest_layer_active();
    widget_state.r_mods        = 0;

    render_status_canvas(&widget_state);
    render_mod_canvas(&widget_state);
    render_blank_canvas();

    return 0;
}

lv_obj_t *blecorne_peripheral_widget_obj(struct blecorne_peripheral_widget *widget) {
    return widget->obj;
}
