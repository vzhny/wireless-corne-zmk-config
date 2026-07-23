#pragma once

#include <lvgl.h>

/* Same source as pixel_operator_mono.h (PixelOperatorMono.ttf, --bpp 1),
 * converted a second time at --size 20 (line_height 20, adv_w 10px/char)
 * instead of --size 16 (13, 8px/char) - layer name only, big enough to read
 * at a glance without needing the full 68px canvas width. */
extern const lv_font_t pixel_operator_mono_large;
