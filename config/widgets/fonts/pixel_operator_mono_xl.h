#pragma once

#include <lvgl.h>

/* Same source as pixel_operator_mono_large.h (PixelOperatorMono.ttf, --bpp 1),
 * converted at --size 24 (12px/char) - an exact 2x of the font's pixel grid,
 * so every stroke is a clean 2px. Used for the central's layer name only:
 * the longest name ("Admin", 5 chars) is 60px, leaving 4px each side in the
 * 68px middle canvas. Size 26+ clips. */
extern const lv_font_t pixel_operator_mono_xl;
