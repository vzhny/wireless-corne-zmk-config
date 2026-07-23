#pragma once

#include <lvgl.h>

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
