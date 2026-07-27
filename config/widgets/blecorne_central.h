#pragma once

#include <lvgl.h>

/* Base-profile layer indices (blecorne.keymap order) - shared with
 * modifier_sync_central.c so both the local display and the peripheral GATT
 * payload derive is_mac/is_colemak the same way: checking whether these
 * specific profile layers are active, not zmk_keymap_highest_layer_active()
 * (which breaks once NUM/NAV/SYM/FUNC/ADMIN stack on top of a profile - see
 * blecorne_central.c). Update if the keymap's layer order ever changes. */
#define LAYER_QWERTY_MAC  2
#define LAYER_COLEMAK_WIN 3
#define LAYER_COLEMAK_MAC 4

struct blecorne_central_widget {
    lv_obj_t *obj;
};

int blecorne_central_widget_init(struct blecorne_central_widget *widget, lv_obj_t *parent);
lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget);

/* Real HID mods OR'd with the display-only shadow-tracked mods (see
 * blecorne_central.c's shadow-tracking section) - used by
 * modifier_sync_central.c to forward the right-hand nibble to the
 * peripheral. */
uint8_t blecorne_central_get_display_mods(void);
