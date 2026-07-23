#pragma once

#include <lvgl.h>

/* Converted from JetBrainsMonoNLNerdFont-Regular.ttf via lv_font_conv
 * (--bpp 1 --size 24) - Nerd Fonts (github.com/ryanoasis/nerd-fonts, MIT)
 * patches Material Design Icons + Font Awesome glyphs into the Private Use
 * Area of any base font. bpp=1 keeps these crisp on the monochrome nice!view
 * (see pixel_operator_mono.h for the same reasoning applied to text).
 *
 * Modifier-row glyphs only, sized for the 28px modifier cells. The
 * status-row icons (BT/wifi/battery/bolt) are a separate, smaller font -
 * see status_icon_font.h. */
extern const lv_font_t icon_font;

/* UTF-8 string literals for the codepoints in icon_font.c - use directly
 * as canvas_draw_text() text with &icon_font. When picking a replacement
 * for any of these, render and compare candidates side by side
 * (fillText on a @font-face'd span, or similar) rather than trusting a
 * glyph's name - "looks about right" has been wrong more than once here. */
#define ICON_SHIFT   "\xf3\xb0\x81\x9d" /* U+F005D md-arrow_up */
#define ICON_CTRL    "\xf3\xb0\x98\xb4" /* U+F0634 md-apple_keyboard_control */
#define ICON_CMD     "\xf3\xb0\x98\xb3" /* U+F0633 md-apple_keyboard_command */
#define ICON_OPT     "\xf3\xb0\x98\xb5" /* U+F0635 md-apple_keyboard_option */

/* Each glyph's real ink width and left bearing (box_w/ofs_x from
 * icon_font.c) - needed to center these icons manually, since
 * LV_TEXT_ALIGN_CENTER centers by the font's shared advance width, not
 * each glyph's actual ink (see draw_mod_icon() in blecorne_central.c). */
#define ICON_SHIFT_W 16
#define ICON_CTRL_W  16
#define ICON_CMD_W   20
#define ICON_OPT_W   18

#define ICON_SHIFT_OFS_X 0
#define ICON_CTRL_OFS_X  0
#define ICON_CMD_OFS_X   0
#define ICON_OPT_OFS_X   0
