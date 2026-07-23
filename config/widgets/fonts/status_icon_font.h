#pragma once

#include <lvgl.h>

/* Converted from JetBrainsMonoNLNerdFont-Regular.ttf via lv_font_conv
 * (--bpp 1 --size 18) - same Nerd Fonts source as icon_font.h, but a separate,
 * smaller conversion for the status row (canvas_bot), which has a different
 * footprint than the modifier row's 28px cells - see icon_font.h. */
extern const lv_font_t status_icon_font;

#define ICON_WIFI            "\xef\x87\xab"     /* U+F1EB  fa-wifi */
#define ICON_BT              "\xef\x8a\x94"     /* U+F294  fa-bluetooth_b */
#define ICON_BATTERY_FULL    "\xef\x89\x80"     /* U+F240  fa-battery_full        76-100% */
#define ICON_BATTERY_3_4     "\xef\x89\x81"     /* U+F241  fa-battery_three_quarters 51-75% */
#define ICON_BATTERY_HALF    "\xef\x89\x82"     /* U+F242  fa-battery_half        26-50% */
#define ICON_BATTERY_QUARTER "\xef\x89\x83"     /* U+F243  fa-battery_quarter      6-25% */
#define ICON_BATTERY_EMPTY   "\xef\x89\x84"     /* U+F244  fa-battery_empty        1-5%, blinks */
/* fa-flash is this Nerd Font's name for Font Awesome's bolt glyph (FA4-era
 * name, kept as-is by the patcher) - a previous round mistakenly used
 * md-bolt (U+F0DB3) instead, which is a screw/fastener icon, not a
 * lightning bolt - fixed to the real bolt glyph. */
#define ICON_BOLT            "\xef\x83\xa7"     /* U+F0E7  fa-flash (Font Awesome's bolt icon) */
