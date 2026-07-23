/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --size 18 --bpp 1 --format lvgl --no-compress --font JetBrainsMonoNLNerdFont-Regular.ttf -r 0xF1EB,0xF294,0xF240,0xF241,0xF242,0xF243,0xF244,0xF0E7 --lv-font-name status_icon_font -o config/widgets/fonts/status_icon_font.c
 ******************************************************************************/

#include <lvgl.h>

#ifndef STATUS_ICON_FONT
#define STATUS_ICON_FONT 1
#endif

#if STATUS_ICON_FONT

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F0E7 "" */
    0x0, 0x40, 0xe, 0x0, 0xf0, 0xf, 0x80, 0xf8,
    0xf, 0xc0, 0xfc, 0xf, 0xfe, 0xff, 0xf3, 0xff,
    0x83, 0xf8, 0x1f, 0x81, 0xf8, 0xf, 0x0, 0x70,
    0x7, 0x0, 0x30, 0x0,

    /* U+F1EB "" */
    0x1, 0xfc, 0x0, 0x7f, 0xfc, 0xf, 0x80, 0xf8,
    0xe0, 0x0, 0xee, 0x0, 0x3, 0xa0, 0xfc, 0x0,
    0x1f, 0xfc, 0x1, 0xe0, 0xf0, 0xc, 0x1, 0x80,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xe0, 0x0,
    0xf, 0x80, 0x0, 0x7c, 0x0, 0x1, 0xc0, 0x0,

    /* U+F240 "" */
    0x7f, 0xff, 0x9f, 0xff, 0xfb, 0x0, 0x3, 0x60,
    0x0, 0x6d, 0xff, 0xcf, 0xbf, 0xf9, 0xf7, 0xff,
    0x3e, 0xff, 0xe7, 0xc0, 0x0, 0xdf, 0xff, 0xf9,
    0xff, 0xfe, 0x0,

    /* U+F241 "" */
    0x7f, 0xff, 0x9f, 0xff, 0xfb, 0x0, 0x3, 0x60,
    0x0, 0x6d, 0xfe, 0xf, 0xbf, 0xc1, 0xf7, 0xf8,
    0x3e, 0xff, 0x7, 0xc0, 0x0, 0xdf, 0xff, 0xf9,
    0xff, 0xfe, 0x0,

    /* U+F242 "" */
    0x7f, 0xff, 0x9f, 0xff, 0xfb, 0x0, 0x3, 0x60,
    0x0, 0x6d, 0xf8, 0xf, 0xbf, 0x1, 0xf7, 0xe0,
    0x3e, 0xfc, 0x7, 0xc0, 0x0, 0xdf, 0xff, 0xf9,
    0xff, 0xfe, 0x0,

    /* U+F243 "" */
    0x7f, 0xff, 0x9f, 0xff, 0xfb, 0x0, 0x3, 0x60,
    0x0, 0x6d, 0xc0, 0xf, 0xb8, 0x1, 0xf7, 0x0,
    0x3e, 0xe0, 0x7, 0xc0, 0x0, 0xdf, 0xff, 0xf9,
    0xff, 0xfe, 0x0,

    /* U+F244 "" */
    0x7f, 0xff, 0x9f, 0xff, 0xfb, 0x0, 0x3, 0x60,
    0x0, 0x6c, 0x0, 0xf, 0x80, 0x1, 0xf0, 0x0,
    0x3e, 0x0, 0x7, 0xc0, 0x0, 0xdf, 0xff, 0xf9,
    0xff, 0xfe, 0x0,

    /* U+F294 "" */
    0x0, 0x8, 0xc, 0xe, 0x4b, 0xeb, 0x7e, 0x3c,
    0x18, 0x1c, 0x3e, 0x6b, 0xcb, 0xe, 0xc, 0x8,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 173, .box_w = 13, .box_h = 17, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 28, .adv_w = 173, .box_w = 21, .box_h = 15, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 68, .adv_w = 173, .box_w = 19, .box_h = 11, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 95, .adv_w = 173, .box_w = 19, .box_h = 11, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 122, .adv_w = 173, .box_w = 19, .box_h = 11, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 149, .adv_w = 173, .box_w = 19, .box_h = 11, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 176, .adv_w = 173, .box_w = 19, .box_h = 11, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 203, .adv_w = 173, .box_w = 8, .box_h = 17, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x104, 0x159, 0x15a, 0x15b, 0x15c, 0x15d, 0x1ad
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 61671, .range_length = 430, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 8, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
const lv_font_t status_icon_font = {
#else
lv_font_t status_icon_font = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -3,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if STATUS_ICON_FONT*/

