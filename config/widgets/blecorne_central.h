#pragma once

#include <lvgl.h>

struct blecorne_central_widget {
    lv_obj_t *obj;
};

int blecorne_central_widget_init(struct blecorne_central_widget *widget, lv_obj_t *parent);
lv_obj_t *blecorne_central_widget_obj(struct blecorne_central_widget *widget);
