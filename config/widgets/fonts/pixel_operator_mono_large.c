/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: --size 20 --bpp 1 --format lvgl --no-compress --font PixelOperatorMono.ttf -r 0x20-0x7F --lv-font-name pixel_operator_mono_large -o config/widgets/fonts/pixel_operator_mono_large.c
 ******************************************************************************/

#include <lvgl.h>

#ifndef PIXEL_OPERATOR_MONO_LARGE
#define PIXEL_OPERATOR_MONO_LARGE 1
#endif

#if PIXEL_OPERATOR_MONO_LARGE

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0x90,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x44, 0x44, 0x44, 0xff, 0x44, 0x44, 0x44, 0x44,
    0xff, 0x44, 0x44, 0x44,

    /* U+0024 "$" */
    0x11, 0xe1, 0x25, 0x92, 0x49, 0x1e, 0x14, 0x51,
    0x65, 0x78, 0x41, 0x4,

    /* U+0025 "%" */
    0x40, 0x0, 0x0, 0x12, 0x44, 0x40, 0x0, 0x20,
    0x24, 0x0, 0x22, 0x40, 0x0, 0x40,

    /* U+0026 "&" */
    0x78, 0x8, 0x60, 0x82, 0x7, 0xa1, 0x86, 0x18,
    0x5f,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x21, 0x49, 0x24, 0x81, 0x10,

    /* U+0029 ")" */
    0x81, 0x12, 0x49, 0x21, 0x40,

    /* U+002A "*" */
    0x12, 0x51, 0x1e, 0x94, 0x40,

    /* U+002B "+" */
    0x10, 0x4f, 0xc4, 0x10, 0x40,

    /* U+002C "," */
    0x48,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x24, 0x94, 0x92, 0x92, 0x40,

    /* U+0030 "0" */
    0x78, 0x8, 0x61, 0x8e, 0x19, 0x79, 0x86, 0x18,
    0x5e,

    /* U+0031 "1" */
    0x10, 0x47, 0x24, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+0032 "2" */
    0x78, 0x8, 0x40, 0x8, 0x1, 0x10, 0x82, 0x8,
    0x3f,

    /* U+0033 "3" */
    0x78, 0x8, 0x41, 0x4, 0x13, 0x81, 0x4, 0x18,
    0x5e,

    /* U+0034 "4" */
    0x4, 0x10, 0xc5, 0x46, 0x18, 0x7f, 0x4, 0x10,
    0x41,

    /* U+0035 "5" */
    0xfe, 0x8, 0x20, 0xf8, 0x0, 0x41, 0x4, 0x18,
    0x5e,

    /* U+0036 "6" */
    0x78, 0x8, 0x60, 0x82, 0xf, 0xa1, 0x86, 0x18,
    0x5e,

    /* U+0037 "7" */
    0xfc, 0x10, 0x41, 0x8, 0x1, 0x10, 0x82, 0x8,
    0x20,

    /* U+0038 "8" */
    0x78, 0x8, 0x61, 0x86, 0x17, 0xa1, 0x86, 0x18,
    0x5e,

    /* U+0039 "9" */
    0x78, 0x8, 0x61, 0x86, 0x17, 0xc1, 0x4, 0x18,
    0x5e,

    /* U+003A ":" */
    0x80, 0x80,

    /* U+003B ";" */
    0x40, 0x0, 0x12,

    /* U+003C "<" */
    0x2a, 0x4, 0x40,

    /* U+003D "=" */
    0xf8, 0x1, 0xf0,

    /* U+003E ">" */
    0x88, 0x85, 0x0,

    /* U+003F "?" */
    0x78, 0x8, 0x40, 0x8, 0x1, 0x4, 0x10, 0x40,
    0x4,

    /* U+0040 "@" */
    0x7e, 0x0, 0x20, 0x31, 0x99, 0x4c, 0xa6, 0x53,
    0x29, 0x8e, 0x40, 0x20, 0xf, 0xc0,

    /* U+0041 "A" */
    0x78, 0x8, 0x61, 0x86, 0x18, 0x7f, 0x86, 0x18,
    0x61,

    /* U+0042 "B" */
    0xfa, 0x8, 0x61, 0x86, 0x1f, 0xa1, 0x86, 0x18,
    0x7e,

    /* U+0043 "C" */
    0x78, 0x8, 0x60, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x5e,

    /* U+0044 "D" */
    0xfa, 0x8, 0x61, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x7e,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x82, 0xf, 0x20, 0x82, 0x8,
    0x3f,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x82, 0xf, 0x20, 0x82, 0x8,
    0x20,

    /* U+0047 "G" */
    0x78, 0x8, 0x60, 0x82, 0x8, 0xe1, 0x86, 0x18,
    0x5f,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0x86, 0x1f, 0xe1, 0x86, 0x18,
    0x61,

    /* U+0049 "I" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+004A "J" */
    0x1f, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
    0x4, 0x4, 0x84, 0x78,

    /* U+004B "K" */
    0x86, 0x8, 0x22, 0x92, 0xe, 0x24, 0x8a, 0x8,
    0x21,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x3f,

    /* U+004D "M" */
    0x80, 0xc0, 0x60, 0x3c, 0x79, 0x4c, 0x6, 0x23,
    0x1, 0x80, 0xc0, 0x60, 0x30, 0x10,

    /* U+004E "N" */
    0x83, 0x6, 0xc, 0x1e, 0x30, 0x64, 0xc7, 0x83,
    0x6, 0xc, 0x10,

    /* U+004F "O" */
    0x78, 0x8, 0x61, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+0050 "P" */
    0xfa, 0x8, 0x61, 0x86, 0x1f, 0xa0, 0x82, 0x8,
    0x20,

    /* U+0051 "Q" */
    0x78, 0x8, 0x61, 0x86, 0x18, 0x61, 0x96, 0x8,
    0x9d,

    /* U+0052 "R" */
    0xfa, 0x8, 0x61, 0x86, 0x1f, 0xa4, 0x8a, 0x8,
    0x21,

    /* U+0053 "S" */
    0x78, 0x8, 0x60, 0x82, 0x7, 0x81, 0x4, 0x18,
    0x5e,

    /* U+0054 "T" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x4,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x10,
    0x1e,

    /* U+0056 "V" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x84, 0x4,
    0x84,

    /* U+0057 "W" */
    0x80, 0xc0, 0x60, 0x31, 0x18, 0x8c, 0x46, 0x23,
    0x11, 0x88, 0xc4, 0x40, 0xe, 0xe0,

    /* U+0058 "X" */
    0x86, 0x18, 0x61, 0x48, 0x1, 0x12, 0x86, 0x18,
    0x61,

    /* U+0059 "Y" */
    0x86, 0x18, 0x61, 0x48, 0x1, 0x4, 0x10, 0x41,
    0x4,

    /* U+005A "Z" */
    0xfc, 0x10, 0x41, 0x8, 0x1, 0x10, 0x82, 0x8,
    0x3f,

    /* U+005B "[" */
    0xf8, 0x88, 0x88, 0x88, 0x88, 0x8f,

    /* U+005C "\\" */
    0x92, 0x44, 0x92, 0x24, 0x90,

    /* U+005D "]" */
    0xf1, 0x11, 0x11, 0x11, 0x11, 0x1f,

    /* U+005E "^" */
    0x10, 0x4, 0xa1,

    /* U+005F "_" */
    0xff, 0xc0,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x7a, 0x10, 0x41, 0x7c, 0x18, 0x41, 0x7c,

    /* U+0062 "b" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x3e,

    /* U+0063 "c" */
    0x7a, 0x18, 0x20, 0x82, 0x8, 0x21, 0x78,

    /* U+0064 "d" */
    0x4, 0x10, 0x5f, 0x86, 0x18, 0x61, 0x86, 0x10,
    0x5f,

    /* U+0065 "e" */
    0x78, 0x8, 0x61, 0xfe, 0x8, 0x21, 0x78,

    /* U+0066 "f" */
    0x38, 0x10, 0x8f, 0xa1, 0x8, 0x42, 0x10, 0x80,

    /* U+0067 "g" */
    0x7c, 0x18, 0x61, 0x86, 0x18, 0x5f, 0x4, 0x18,
    0x5e,

    /* U+0068 "h" */
    0x82, 0x8, 0x3e, 0x82, 0x18, 0x61, 0x86, 0x18,
    0x61,

    /* U+0069 "i" */
    0x10, 0x0, 0x3c, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+006A "j" */
    0x4, 0x0, 0xf, 0x4, 0x10, 0x41, 0x4, 0x10,
    0x41, 0x6, 0x17, 0x80,

    /* U+006B "k" */
    0x82, 0x8, 0x21, 0x8a, 0x9, 0x38, 0x92, 0x8,
    0xa1,

    /* U+006C "l" */
    0xf0, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+006D "m" */
    0xf7, 0x40, 0x22, 0x31, 0x18, 0x8c, 0x46, 0x23,
    0x1, 0x80, 0x80,

    /* U+006E "n" */
    0xfa, 0x8, 0x61, 0x86, 0x18, 0x61, 0x84,

    /* U+006F "o" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x40, 0x78,

    /* U+0070 "p" */
    0xfa, 0x8, 0x61, 0x86, 0x18, 0x61, 0xfa, 0x8,
    0x20,

    /* U+0071 "q" */
    0x7c, 0x18, 0x61, 0x86, 0x18, 0x61, 0x7c, 0x10,
    0x41,

    /* U+0072 "r" */
    0x8e, 0x48, 0x38, 0x82, 0x8, 0x20, 0x80,

    /* U+0073 "s" */
    0x78, 0x8, 0x40, 0x78, 0x0, 0x21, 0x78,

    /* U+0074 "t" */
    0x47, 0xd0, 0x84, 0x21, 0x8, 0x41, 0xc0,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x40, 0x78,

    /* U+0076 "v" */
    0x86, 0x18, 0x61, 0x86, 0x10, 0x12, 0x10,

    /* U+0077 "w" */
    0x80, 0xc0, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x22,
    0x0, 0x77, 0x0,

    /* U+0078 "x" */
    0x84, 0x0, 0x12, 0x11, 0x20, 0x0, 0x84,

    /* U+0079 "y" */
    0x86, 0x18, 0x61, 0x86, 0x10, 0x5f, 0x4, 0x18,
    0x5e,

    /* U+007A "z" */
    0xfc, 0x10, 0x42, 0x11, 0x8, 0x20, 0xfc,

    /* U+007B "{" */
    0x38, 0x10, 0x84, 0x22, 0x8, 0x42, 0x10, 0x70,

    /* U+007C "|" */
    0xff, 0xf0,

    /* U+007D "}" */
    0xe0, 0x4, 0x21, 0x8, 0x22, 0x10, 0x85, 0xc0,

    /* U+007E "~" */
    0x73, 0x18
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 160, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 160, .box_w = 1, .box_h = 12, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 160, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 5, .adv_w = 160, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 160, .box_w = 6, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 29, .adv_w = 160, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 160, .box_w = 1, .box_h = 4, .ofs_x = 4, .ofs_y = 8},
    {.bitmap_index = 53, .adv_w = 160, .box_w = 3, .box_h = 12, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 160, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 160, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 68, .adv_w = 160, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 73, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 74, .adv_w = 160, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 75, .adv_w = 160, .box_w = 1, .box_h = 1, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 160, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 160, .box_w = 1, .box_h = 9, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 160, .box_w = 2, .box_h = 12, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 176, .adv_w = 160, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 179, .adv_w = 160, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 182, .adv_w = 160, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 185, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 160, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 160, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 160, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 160, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 353, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 160, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 439, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 448, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 160, .box_w = 4, .box_h = 12, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 160, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 160, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 474, .adv_w = 160, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 477, .adv_w = 160, .box_w = 10, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 479, .adv_w = 160, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 480, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 496, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 519, .adv_w = 160, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 536, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 160, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 566, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 575, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 584, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 602, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 618, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 627, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 641, .adv_w = 160, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 648, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 662, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 680, .adv_w = 160, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 689, .adv_w = 160, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 696, .adv_w = 160, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 704, .adv_w = 160, .box_w = 1, .box_h = 12, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 706, .adv_w = 160, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 714, .adv_w = 160, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = 9}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t pixel_operator_mono_large = {
#else
lv_font_t pixel_operator_mono_large = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -4,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if PIXEL_OPERATOR_MONO_LARGE*/

