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
 * as text now (see render_mod_canvas), so no icon ever referenced it.
 *
 * ICON_SHIFT used to be md-apple_keyboard_shift (U+F0636) - a solid/filled
 * arrow-with-roof shape, visibly heavier-stroked than the "Ctl"/"Win"/"Alt"
 * text and the other 3 (thin-lined) icons once actually seen on real
 * hardware - this font has no outlined/thin variant of that specific
 * house-shaped glyph. First tried md-chevron_up (U+F0143, thin, matches
 * weight) but it and ICON_CTRL (also a simple "^" shape) then looked nearly
 * identical at this size - hard to tell the two mod cells apart at a
 * glance. Settled on fa-angles_up (U+F102, a double chevron "^^") - still
 * thin-stroked, but visually distinct from Ctrl's single caret. */
#define ICON_SHIFT   "\xef\x84\x82" /* U+F102  fa-angles_up */
#define ICON_CTRL    "\xf3\xb0\x98\xb4" /* U+F0634 md-apple_keyboard_control */
#define ICON_CMD     "\xf3\xb0\x98\xb3" /* U+F0633 md-apple_keyboard_command */
#define ICON_OPT     "\xf3\xb0\x98\xb5" /* U+F0635 md-apple_keyboard_option */

/* Each glyph's actual ink width (LVGL's lv_font_fmt_txt_glyph_dsc_t.box_w),
 * needed to manually center these icons - see the comment on draw_mod_icon()
 * in blecorne_central.c/blecorne_peripheral.c for why LV_TEXT_ALIGN_CENTER
 * alone doesn't work here. All 4 have ofs_x=0 (no left bearing to account
 * for) - ICON_*_OFS_X kept (all 0) so draw_mod_icon()'s centering math
 * doesn't need a special case if a future icon swap does have bearing. */
#define ICON_SHIFT_W 17
#define ICON_CTRL_W  16
#define ICON_CMD_W   20
#define ICON_OPT_W   18

#define ICON_SHIFT_OFS_X 0
#define ICON_CTRL_OFS_X  0
#define ICON_CMD_OFS_X   0
#define ICON_OPT_OFS_X   0
