#pragma once

#include <lvgl.h>

/* Converted from PixelOperatorMono.ttf via lv_font_conv (--bpp 1 --size 16,
 * ASCII 0x20-0x7F) - a purpose-built pixel font, crisp on this project's
 * strictly monochrome Sharp Memory LCD in a way LVGL's built-in Montserrat
 * (bpp 4, antialiased) isn't - antialiased edges get thresholded and look
 * soft on a 1-bit display. Used for the "Ctl"/"Win"/"Alt" mod-cell text
 * only - see pixel_operator_mono_large.h for everything else. */
extern const lv_font_t pixel_operator_mono;
