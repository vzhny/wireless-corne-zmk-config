#pragma once

#include <lvgl.h>

/* 14×14 L8 bitmap image descriptors for modifier key glyphs */

/* ⊞  Windows logo (4-pane grid) */
extern const lv_image_dsc_t glyph_win;

/* ⌘  Command */
extern const lv_image_dsc_t glyph_cmd;

/* ⌃  Control (caret, shared Win+Mac) */
extern const lv_image_dsc_t glyph_ctrl;

/* ⌥  Option */
extern const lv_image_dsc_t glyph_opt;

/* ⎇  Alt (upward arrow with base bar) */
extern const lv_image_dsc_t glyph_alt;

/* ⇧  Shift (solid upward arrow with stem and base) */
extern const lv_image_dsc_t glyph_sft;

/* ᛒ  Bluetooth (hagall-rune style: vertical spine + two mirrored V arms) */
extern const lv_image_dsc_t glyph_bt;
