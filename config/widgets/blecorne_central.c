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

/* ── Canvases ────────────────────────────────────────────────────────── */

static lv_obj_t *canvas_top;    /* x=-44 → physical bottom strip, 24px visible (layer name) */
static lv_obj_t *canvas_mid;    /* x=24  → physical middle strip (modifiers)                */
static lv_obj_t *canvas_bot;    /* x=92 (TOP_RIGHT) → physical top strip (status)           */

static uint8_t cbuf_top[CANVAS_BUF_SIZE];
static uint8_t cbuf_mid[CANVAS_BUF_SIZE];
static uint8_t cbuf_bot[CANVAS_BUF_SIZE];

/* ── Layer names ─────────────────────────────────────────────────────── */

/* Layers 0-3 (Qwerty/Colemak x Win/Mac) all show "Base" - which keyboard
 * layout is active is shown on the peripheral's layout row instead (see
 * blecorne_peripheral.c), and Win/Mac is already shown by the modifier
 * row's icons-vs-text (see render_mod_canvas), so the name itself doesn't
 * need to carry either distinction. Also needed room for the larger layer
 * name font - "Colemak (Win)"-style names never fit regardless of size. */
static const char *layer_names[] = {
    "Base", "Base",
    "Base", "Base",
    /* Func(7)/Admin(8) order matches blecorne.keymap - ADMIN must be the
     * higher index there so zmk_keymap_highest_layer_active() resolves to
     * it (not Func) when both are simultaneously active. */
    "Num", "Nav", "Sym", "Func", "Admin",
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

/* ── Flash state (500ms heartbeat, always running) ───────────────────── *
 * Drives two independent blinks: the BT glyph while searching (bt_connected
 * false) and the battery-empty glyph at <=5% (regardless of BT state) - one
 * shared timer since both just need "on/off every 500ms", not a per-purpose
 * schedule. Runs continuously rather than starting/stopping with BT state
 * (used to stop on connect) since battery-empty needs it whether or not BT
 * is connected. */

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

/* status_icon_font glyph (status row). `align` picks LEFT for the BT/wifi
 * icon (hugs the left edge) or RIGHT for the battery/bolt icon (hugs the
 * right edge, stacking directly above the right-aligned % text below it). */
static void draw_status_icon(lv_obj_t *canvas, int x, int y, int w, const char *icon,
                             bool active, lv_text_align_t align) {
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, LVGL_FOREGROUND, &status_icon_font);
    dsc.align = align;
    dsc.opa   = active ? LV_OPA_COVER : LV_OPA_40;
    canvas_draw_text(canvas, x, y, w, &dsc, icon);
}

/* ── Modifier cell (border box, invert-on-press) ──────────────────────── *
 * Each modifier key is a 1px rounded-border box, always visible. Pressed
 * ("active") state is shown by filling the box with foreground color and
 * inverting the icon/text color inside it (a "lit key" look), replacing the
 * old plain opacity fade. */

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

/* `icon_w` is the glyph's real ink width (ICON_*_W in icon_font.h), NOT the
 * box width. LV_TEXT_ALIGN_CENTER alone under-centers these: lv_font_conv
 * gives every glyph in this font the same adv_w (a made-up "monospace"
 * advance, ~14px), but the actual icons are wider than that (12-20px) - so
 * centering-by-advance-width places them noticeably off-center relative to
 * their true ink. Centering manually against the real box_w instead (LEFT
 * align at a computed x) fixes it. `icon_ofs_x` is the glyph's left bearing
 * (ICON_*_OFS_X) - LVGL adds this on top of wherever we tell it to draw, so
 * it has to be subtracted back out of the centering math or the glyph
 * drifts right by that amount (only nonzero for Shift, currently). */
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
 * the caller separately (blinks via ICON_BATTERY_EMPTY, not shown solid). */
static const char *battery_icon(uint8_t level) {
    if (level > 75) return ICON_BATTERY_FULL;
    if (level > 50) return ICON_BATTERY_3_4;
    if (level > 25) return ICON_BATTERY_HALF;
    return ICON_BATTERY_QUARTER;
}

/* Friendly 2-character names for specific BT profile slots, so the status
 * row can show e.g. "WL" instead of "B0" for profiles the user re-pairs to
 * the same device every time. Profiles without an alias here just fall
 * back to "B<n>". Update this table (not the keymap) if a profile gets
 * re-paired to a different device. */
static const char *profile_alias(uint8_t profile) {
    switch (profile) {
        case 0: return "WL"; /* Work laptop */
        case 1: return "MB"; /* MacBook */
        case 2: return "iP"; /* iPad */
        default: return NULL;
    }
}

/* ── Canvas renderers ────────────────────────────────────────────────── */

/* canvas_bot (physical top strip) - NOT hardware-truncated (unlike
 * canvas_top), full 68px budget available; only the first ~36 are used.
 *
 * Two rows: icons on row 1 (y=0), text on row 2 (y=20) - status_icon_font at
 * size 18 (up from 12) needs more headroom than the old 24-row convention
 * left, and this canvas has the room to spare. BT/"B n" left-aligned,
 * battery/bolt/% right-aligned, `pixel_operator_mono_large` (same size as
 * the layer name) for both, per row - see the field-width comment below for
 * why the BT-profile label had to shrink to fit at that size.
 *
 * Battery icon (status_icon_font, real Font Awesome glyphs, not the old
 * hand-drawn shell+fill rects) is one of five discrete levels rather than a
 * continuous fill - see battery_icon(). At <=5% it's ICON_BATTERY_EMPTY,
 * shown only while flash_on (blinks). While charging, the battery icon is
 * replaced entirely by ICON_BOLT - no battery glyph drawn at all.
 */
static void render_status_canvas(struct central_state *state) {
    clear_canvas(canvas_bot);

    lv_draw_label_dsc_t lbl;
    init_label_dsc(&lbl, LVGL_FOREGROUND, &pixel_operator_mono_large);

    /* Row 1 (y=0): BT icon top-left, battery/bolt icon top-right.
     * BT icon is always visible (never disappears) - dim/grey when not
     * connected (matching the peripheral's dim-wifi-when-disconnected
     * treatment - see draw_wifi_icon in blecorne_peripheral.c), full white
     * once connected. While searching (not yet connected) it blinks
     * dim/bright every 500ms via flash_on instead of just sitting dim -
     * that blink alone signals "still pairing", no separate text needed. */
    draw_status_icon(canvas_bot, 0, 0, 24, ICON_BT, bt_connected || flash_on, LV_TEXT_ALIGN_LEFT);

    /* x=40 (was 44) - nudged slightly left of the canvas edge per real-
     * hardware feedback; the top edge (y=0) was already fine. */
    if (state->charging) {
        draw_status_icon(canvas_bot, 40, 0, 24, ICON_BOLT, true, LV_TEXT_ALIGN_RIGHT);
    } else if (state->battery_level <= 5) {
        if (flash_on) {
            draw_status_icon(canvas_bot, 40, 0, 24, ICON_BATTERY_EMPTY, true, LV_TEXT_ALIGN_RIGHT);
        }
    } else {
        draw_status_icon(canvas_bot, 40, 0, 24, battery_icon(state->battery_level), true, LV_TEXT_ALIGN_RIGHT);
    }

    /* Row 2 (y=20): BT profile left (under the BT icon) once connected,
     * otherwise blank; battery % right (under the battery icon), always.
     * pixel_operator_mono_large is a fixed 10px/char, so both fields have to
     * be sized off exact character counts, not guessed - "100%" (4 chars) is
     * 40px wide, leaving only 28px for the left field if the two are to
     * have any gap at all in the worst case. "BT %d" (5 chars, 50px) simply
     * doesn't fit at this size - shortened to "B%d" (max "B4", 2 chars,
     * 20px) so there's still 6px of daylight between the two fields. */
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

/* canvas_mid (physical middle strip, 68 px)
 *
 * Two rows of bordered modifier cells (MOD_BOX_W x MOD_BOX_H, see
 * draw_mod_box()) - Shift+Ctrl on row 1 (y=6), GUI+Alt on row 2 (y=38),
 * columns at x=4/x=36.
 *
 * Mac: all four cells are icon_font glyphs (real Material Design "Apple
 * keyboard" + Font Awesome artwork via Nerd Fonts) - Shift, Ctrl, Cmd, Opt.
 * Windows: only Shift stays an icon. Ctrl/GUI/Alt become plain text ("Ctl"/
 * "Win"/"Alt") instead, because real Windows keyboards print those as text
 * on the keycap, not a symbol - unlike Mac, which does print ⌘/⌥/⌃ symbols.
 * "Ctrl" (4 chars) wrapped onto a second line at this font/box size - "Ctl"
 * (3 chars, matching "Win"/"Alt") fits on one line like the other two. */
static void render_mod_canvas(struct central_state *state) {
    clear_canvas(canvas_mid);

    /* Section separators: this canvas occupies its full 68px budget with no
     * hardware truncation (unlike status/layer), so a rule at y=0 and y=67
     * lands exactly on the true physical boundary with the status strip
     * above and the layer strip below - see CLAUDE.md's Canvas layout notes. */
    lv_draw_rect_dsc_t rule_dsc;
    init_rect_dsc(&rule_dsc, LVGL_FOREGROUND);
    canvas_draw_rect(canvas_mid, 0, 0, CANVAS_SIZE, 1, &rule_dsc);
    canvas_draw_rect(canvas_mid, 0, CANVAS_SIZE - 1, CANVAS_SIZE, 1, &rule_dsc);

    bool is_mac = (state->active_layer == 1 || state->active_layer == 3);
    uint32_t mods = state->mods;

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

/* canvas_top (physical bottom strip, 24 px visible = canvas rows 0..23) -
 * layer name only, no circles. y=5 (not vertically centered in the full 68px
 * canvas) since only rows 0..23 actually reach the screen here. */
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
    render_mod_canvas(&widget_state); /* layer affects Mac vs Win glyph order */
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

    /* flash_timer runs continuously (see its declaration) - no start/stop
     * here. render_status_canvas's `bt_connected || flash_on` already shows
     * the icon solid once connected regardless of flash_on's value. */

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

    render_status_canvas(&widget_state);
    render_mod_canvas(&widget_state);
    render_layer_canvas(&widget_state);

    return 0;
}

lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget) {
    return widget->obj;
}
