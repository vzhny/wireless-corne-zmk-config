#pragma once

#include <lvgl.h>

/* Converted from PixelOperatorMono.ttf via lv_font_conv (--bpp 1 --size 16,
 * ASCII 0x20-0x7F) - a purpose-built pixel font, unlike LVGL's built-in
 * Montserrat (bpp 4, antialiased, meant for color/grayscale screens). On this
 * project's strictly monochrome Sharp Memory LCD, Montserrat's antialiased
 * edges get thresholded and look soft; this font has no antialiasing to lose,
 * so it stays crisp. Used for all text: battery %, BT profile, layer name. */
extern const lv_font_t pixel_operator_mono;
