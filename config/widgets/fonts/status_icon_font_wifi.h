#pragma once

#include <lvgl.h>

/* Peripheral's connection icon, converted at its own size (16, vs
 * status_icon_font's 18) - this is the widest glyph in that font's Nerd
 * Font subset and reads better a size down in its corner. Same source
 * font and codepoint, just a second conversion, since an LVGL font is a
 * fixed size (not scalable at runtime). */
extern const lv_font_t status_icon_font_wifi;

#define ICON_WIFI_SMALL "\xef\x87\xab" /* U+F1EB fa-wifi */
