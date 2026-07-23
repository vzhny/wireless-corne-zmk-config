#pragma once

#include <lvgl.h>

/* Converted from JetBrainsMonoNLNerdFont-Regular.ttf via lv_font_conv
 * (--bpp 1 --size 24) - Nerd Fonts (github.com/ryanoasis/nerd-fonts, MIT)
 * patches Material Design Icons + Font Awesome glyphs into the Private Use
 * Area of any base font. Replaces this project's hand-drawn 0/1 bitmap
 * glyphs (glyphs.c, removed) with real vector icon artwork, rasterized once
 * at bpp=1 - same crisp-on-monochrome reasoning as pixel_operator_mono.
 *
 * Modifier-row glyphs only (~20px, sized for the 28px modifier cells). The
 * status-row icons (BT/wifi/battery/bolt) are a separate, smaller font -
 * see status_icon_font.h - since a single shared size clipped/overflowed
 * the much shorter 24px-tall status strip. */
extern const lv_font_t icon_font;

/* UTF-8 encoded string literals for the specific PUA codepoints pulled into
 * icon_font.c. Use directly as canvas_draw_text() text with &icon_font.
 * fa-windows (U+F17A) was dropped from the conversion - Windows shows "Win"
 * as text now (see render_mod_canvas), so no icon ever referenced it. */
#define ICON_SHIFT   "\xf3\xb0\x98\xb6" /* U+F0636 md-apple_keyboard_shift */
#define ICON_CTRL    "\xf3\xb0\x98\xb4" /* U+F0634 md-apple_keyboard_control */
#define ICON_CMD     "\xf3\xb0\x98\xb3" /* U+F0633 md-apple_keyboard_command */
#define ICON_OPT     "\xf3\xb0\x98\xb5" /* U+F0635 md-apple_keyboard_option */

/* Each glyph's actual ink width (LVGL's lv_font_fmt_txt_glyph_dsc_t.box_w),
 * needed to manually center these icons - see the comment on draw_mod_icon()
 * in blecorne_central.c/blecorne_peripheral.c for why LV_TEXT_ALIGN_CENTER
 * alone doesn't work here. All 4 glyphs have ofs_x=0 (no left bearing to
 * account for too). */
#define ICON_SHIFT_W 20
#define ICON_CTRL_W  16
#define ICON_CMD_W   20
#define ICON_OPT_W   18
