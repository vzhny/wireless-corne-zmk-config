#pragma once

#include <lvgl.h>

/* Same source as pixel_operator_mono.h (PixelOperatorMono.ttf, --bpp 1),
 * converted a second time at --size 20 (10px/char) instead of --size 16
 * (8px/char) - used for anything meant to read at a glance: the layer
 * name, the peripheral's layout name, and the status row's BT profile/
 * battery % text. */
extern const lv_font_t pixel_operator_mono_large;
