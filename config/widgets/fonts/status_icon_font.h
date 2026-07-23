#pragma once

#include <lvgl.h>

/* Converted from JetBrainsMonoNLNerdFont-Regular.ttf via lv_font_conv
 * (--bpp 1 --size 18) - same source as icon_font.h, a separate conversion
 * sized for the status row (canvas_bot) rather than the modifier cells.
 * The wifi/connection icon has its own font instead - see
 * status_icon_font_wifi.h. */
extern const lv_font_t status_icon_font;

#define ICON_BT              "\xef\x8a\x94"     /* U+F294  fa-bluetooth_b */
#define ICON_BATTERY_FULL    "\xef\x89\x80"     /* U+F240  fa-battery_full        76-100% */
#define ICON_BATTERY_3_4     "\xef\x89\x81"     /* U+F241  fa-battery_three_quarters 51-75% */
#define ICON_BATTERY_HALF    "\xef\x89\x82"     /* U+F242  fa-battery_half        26-50% */
#define ICON_BATTERY_QUARTER "\xef\x89\x83"     /* U+F243  fa-battery_quarter      6-25% */
#define ICON_BATTERY_EMPTY   "\xef\x89\x84"     /* U+F244  fa-battery_empty        1-5%, blinks */
#define ICON_BOLT            "\xef\x83\xa7"     /* U+F0E7  fa-flash (Font Awesome's bolt icon) */
