#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/hid.h>
#include <zmk/ble.h>

#include "util.h"
#include "glyphs.h"
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

/* ── Canvases ────────────────────────────────────────────────────────── */

static lv_obj_t *canvas_top;    /* x=92  → physical bottom strip (layer name) */
static lv_obj_t *canvas_mid;    /* x=24  → physical middle strip (modifiers)  */
static lv_obj_t *canvas_bot;    /* x=-44 → physical top strip   (status)      */

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
    return (idx < LAYER_NAME_COUNT) ? layer_names[idx] : "???";
}

/* ── HID modifier masks ──────────────────────────────────────────────── */

#define MOD_LCTRL  BIT(0)
#define MOD_LSHIFT BIT(1)
#define MOD_LALT   BIT(2)
#define MOD_LGUI   BIT(3)

/* ── BT flash state ──────────────────────────────────────────────────── */

static bool    bt_connected    = false;
static bool    bt_flash_on     = true;
static uint8_t connecting_dots = 1; /* 1..3, advances each 500ms tick while searching */

static void bt_flash_work_cb(struct k_work *work);
static K_WORK_DEFINE(bt_flash_work, bt_flash_work_cb);

static void bt_flash_timer_cb(struct k_timer *t) {
    k_work_submit(&bt_flash_work);
}
static K_TIMER_DEFINE(bt_flash_timer, bt_flash_timer_cb, NULL);

/* ── Draw helpers ────────────────────────────────────────────────────── */

static void clear_canvas(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t r;
    init_rect_dsc(&r, LVGL_BACKGROUND);
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &r);
}

/* ── Canvas renderers ────────────────────────────────────────────────── */

/* canvas_bot (physical top strip, 24 px visible = canvas rows 0..23)
 *
 * Layout (canvas space) - confirmed against real hardware:
 *   Physical top row    → canvas y=0..11:   BT glyph x=0, battery x=16, % x=40
 *   Physical bottom row → canvas y=13..23:  BT profile "BT 0"-"BT 3" x=0
 *
 * (Previous comment here had this backwards - canvas_cy=0 is physical top,
 * not bottom, for this rotation.)
 */
static void render_status_canvas(struct central_state *state) {
    clear_canvas(canvas_bot);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &lv_font_montserrat_12);

    /* Physical top row (canvas y=0..11): BT indicator left, battery+% right */
    if (bt_connected || bt_flash_on) {
        draw_glyph(canvas_bot, 0, 0, &glyph_bt, true);
    }
    draw_battery(canvas_bot, 16, 1, state->battery_level, state->charging);
    char batt_buf[6];
    snprintf(batt_buf, sizeof(batt_buf), "%d%%", state->battery_level);
    canvas_draw_text(canvas_bot, 40, 0, 28, &lbl, batt_buf);

    /* Physical bottom row (canvas y≥13): BT profile, or "Connecting..." while searching */
    if (bt_connected) {
        char profile_buf[6];
        snprintf(profile_buf, sizeof(profile_buf), "BT %d", state->ble_profile);
        canvas_draw_text(canvas_bot, 0, 13, 68, &lbl, profile_buf);
    } else {
        lv_draw_label_dsc_t conn_lbl;
        init_label_dsc(&conn_lbl, LVGL_FOREGROUND, &lv_font_montserrat_10);
        char conn_buf[14];
        snprintf(conn_buf, sizeof(conn_buf), "Connecting%.*s", connecting_dots, "...");
        canvas_draw_text(canvas_bot, 0, 14, 68, &conn_lbl, conn_buf);
    }

    rotate_canvas(canvas_bot);
}

/* canvas_mid (physical middle strip, 68 px)
 *
 * Modifier glyphs at canvas (gx, gy=27), step=17 px.
 * Win order: ⇧ ⌃ ⊞ Alt
 * Mac order: ⇧ ⌘ ⌃ ⌥   (GUI and Ctrl swapped relative to Win)
 */
static void render_mod_canvas(struct central_state *state) {
    clear_canvas(canvas_mid);

    bool is_mac = (state->active_layer == 1 || state->active_layer == 3);
    uint32_t mods = state->mods;

    bool shift_active = !!(mods & MOD_LSHIFT);
    bool ctrl_active  = !!(mods & MOD_LCTRL);
    bool gui_active   = !!(mods & MOD_LGUI);
    bool alt_active   = !!(mods & MOD_LALT);

    int gx = 2, gy = 27, step = 17;

    draw_glyph(canvas_mid, gx, gy, &glyph_sft, shift_active);
    if (is_mac) {
        /* Mac: ⇧ ⌘ ⌃ ⌥ */
        draw_glyph(canvas_mid, gx + step,     gy, &glyph_cmd,  gui_active);
        draw_glyph(canvas_mid, gx + step * 2, gy, &glyph_ctrl, ctrl_active);
        draw_glyph(canvas_mid, gx + step * 3, gy, &glyph_opt,  alt_active);
    } else {
        /* Win: ⇧ ⌃ ⊞ Alt */
        draw_glyph(canvas_mid, gx + step,     gy, &glyph_ctrl, ctrl_active);
        draw_glyph(canvas_mid, gx + step * 2, gy, &glyph_win,  gui_active);
        draw_glyph(canvas_mid, gx + step * 3, gy, &glyph_alt,  alt_active);
    }

    rotate_canvas(canvas_mid);
}

/* canvas_top (physical bottom strip, 68 px)
 *
 * Layout (physical strip_y from top, canvas_cy = 67-strip_y):
 *   strip_y=8..17  : circle row 1 — layers 1-5  (canvas y=50, h=10)
 *   strip_y=21..30 : circle row 2 — layers 6-9  (canvas y=37, h=10)
 *   strip_y=40..55 : layer name centred          (canvas y=12, h=16)
 *
 * Circle n (1-indexed) fills when active_layer == n.
 * Circle 9 is always hollow (no layer 9 exists).
 *
 * Row 1 (5 circles, 10px, 3px gap): x = 3, 16, 29, 42, 55
 * Row 2 (4 circles, 10px, 3px gap): x = 10, 23, 36, 49
 */
static void render_layer_canvas(struct central_state *state) {
    clear_canvas(canvas_top);

    uint8_t active = state->active_layer;
    lv_draw_label_dsc_t num_lbl;
    char num_str[3];

    /* Row 1: layers 1–5, x=3,16,29,42,55 at canvas y=50 */
    for (int i = 1; i <= 5; i++) {
        int cx = 3 + (i - 1) * 13;
        bool on = (active == i);
        draw_circle(canvas_top, cx, 50, 10, on);
        init_label_dsc(&num_lbl, on ? LVGL_BACKGROUND : LVGL_FOREGROUND,
                       &lv_font_montserrat_10);
        num_lbl.align = LV_TEXT_ALIGN_CENTER;
        snprintf(num_str, sizeof(num_str), "%d", i);
        canvas_draw_text(canvas_top, cx, 51, 10, &num_lbl, num_str);
    }

    /* Row 2: layers 6–9, x=10,23,36,49 at canvas y=37 (9 always hollow) */
    for (int i = 6; i <= 9; i++) {
        int cx = 10 + (i - 6) * 13;
        bool on = (active == i);
        draw_circle(canvas_top, cx, 37, 10, on);
        init_label_dsc(&num_lbl, on ? LVGL_BACKGROUND : LVGL_FOREGROUND,
                       &lv_font_montserrat_10);
        num_lbl.align = LV_TEXT_ALIGN_CENTER;
        snprintf(num_str, sizeof(num_str), "%d", i);
        canvas_draw_text(canvas_top, cx, 38, 10, &num_lbl, num_str);
    }

    /* Layer name */
    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &lv_font_montserrat_16);
    lbl.align = LV_TEXT_ALIGN_CENTER;
    canvas_draw_text(canvas_top, 0, 12, CANVAS_SIZE, &lbl,
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
    render_mod_canvas(&widget_state); /* layer affects Mac vs Win glyph order */
}
static K_WORK_DEFINE(layer_mod_render_work, layer_mod_render_cb);

static inline void display_submit(struct k_work *work) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), work);
    }
}

/* ── BT flash work item (submitted by timer, runs on system workq) ───── */

static void bt_flash_work_cb(struct k_work *work) {
    bt_flash_on     = !bt_flash_on;
    connecting_dots = (connecting_dots % 3) + 1;
    display_submit(&status_render_work);
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

    if (bt_connected) {
        k_timer_stop(&bt_flash_timer);
        bt_flash_on = true;
    } else {
        bt_flash_on     = true; /* start visible so first visible frame shows icon */
        connecting_dots = 1;    /* restart dot animation at "Connecting." */
        k_timer_start(&bt_flash_timer, K_MSEC(500), K_MSEC(500));
    }

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
    lv_obj_align(canvas_top, LV_ALIGN_TOP_RIGHT, 0, 0);

    canvas_mid = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_mid, cbuf_mid, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_mid, LV_ALIGN_TOP_LEFT, 24, 0);

    canvas_bot = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(canvas_bot, cbuf_bot, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);
    lv_obj_align(canvas_bot, LV_ALIGN_TOP_LEFT, -44, 0);

    bt_connected = zmk_ble_active_profile_is_connected();

    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.charging      = zmk_usb_is_powered();
    widget_state.active_layer  = zmk_keymap_highest_layer_active();
    widget_state.mods          = zmk_hid_get_explicit_mods();
    widget_state.ble_connected = bt_connected;
    widget_state.ble_profile   = zmk_ble_active_profile_index();

    if (!bt_connected) {
        k_timer_start(&bt_flash_timer, K_MSEC(500), K_MSEC(500));
    }

    render_status_canvas(&widget_state);
    render_mod_canvas(&widget_state);
    render_layer_canvas(&widget_state);

    return 0;
}

lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget) {
    return widget->obj;
}
