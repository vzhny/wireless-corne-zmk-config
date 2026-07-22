#pragma once

#include <zephyr/sys/util.h>
#include <lvgl.h>

#define CANVAS_SIZE 68
#define CANVAS_COLOR_FORMAT LV_COLOR_FORMAT_L8
#define CANVAS_BUF_SIZE \
    LV_CANVAS_BUF_SIZE(CANVAS_SIZE, CANVAS_SIZE, \
                       LV_COLOR_FORMAT_GET_BPP(CANVAS_COLOR_FORMAT), \
                       LV_DRAW_BUF_STRIDE_ALIGN)

#define LVGL_BACKGROUND \
    (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white())
#define LVGL_FOREGROUND \
    (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black())

void rotate_canvas(lv_obj_t *canvas);
void draw_battery(lv_obj_t *canvas, int x, int y, uint8_t level, bool charging);
void draw_glyph(lv_obj_t *canvas, int x, int y, const lv_image_dsc_t *glyph, bool active);
void draw_circle(lv_obj_t *canvas, int x, int y, int size, bool filled);

void init_label_dsc(lv_draw_label_dsc_t *dsc, lv_color_t color, const lv_font_t *font);
void init_rect_dsc(lv_draw_rect_dsc_t *dsc, lv_color_t color);
void init_line_dsc(lv_draw_line_dsc_t *dsc, lv_color_t color, uint8_t width);

void canvas_draw_text(lv_obj_t *canvas, int x, int y, int w,
                      lv_draw_label_dsc_t *dsc, const char *txt);
void canvas_draw_rect(lv_obj_t *canvas, int x, int y, int w, int h,
                      lv_draw_rect_dsc_t *dsc);
void canvas_draw_line(lv_obj_t *canvas, lv_point_t points[], uint16_t count,
                      lv_draw_line_dsc_t *dsc);
void canvas_draw_img(lv_obj_t *canvas, int x, int y, const void *src,
                     lv_draw_image_dsc_t *dsc);
