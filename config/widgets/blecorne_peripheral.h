#pragma once

#include <lvgl.h>

struct blecorne_peripheral_widget {
    lv_obj_t *obj;
};

int blecorne_peripheral_widget_init(struct blecorne_peripheral_widget *widget, lv_obj_t *parent);
lv_obj_t *blecorne_peripheral_widget_obj(struct blecorne_peripheral_widget *widget);
/* Called by modifier_sync_peripheral when central writes R-mod state */
void blecorne_peripheral_update_mods(uint8_t r_mods);
