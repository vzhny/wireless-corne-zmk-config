#pragma once

#include <lvgl.h>

/* Peripheral's connection icon only, one size down from status_icon_font
 * (--size 16 vs 18) - at size 18 the wifi glyph (the widest icon in this
 * Nerd Font subset, box_w=21) read as crowding its corner; a size smaller
 * (box_w=18) reads cleaner without needing more room. Same source font and
 * codepoint as ICON_WIFI in status_icon_font.h - just a second, smaller
 * conversion, since LVGL fonts are each a fixed size, not scalable at
 * runtime. */
extern const lv_font_t status_icon_font_wifi;

#define ICON_WIFI_SMALL "\xef\x87\xab" /* U+F1EB fa-wifi */
