#include <string.h>
#include <lvgl.h>
#include <draw/sw/lv_draw_sw_utils.h>
#include "util.h"

void rotate_canvas(lv_obj_t *canvas) {
    uint8_t *buf = lv_canvas_get_draw_buf(canvas)->data;
    static uint8_t buf_copy[CANVAS_BUF_SIZE];
    memcpy(buf_copy, buf, sizeof(buf_copy));
    const uint32_t stride =
        lv_draw_buf_width_to_stride(CANVAS_SIZE, CANVAS_COLOR_FORMAT);
    lv_draw_sw_rotate(buf_copy, buf, CANVAS_SIZE, CANVAS_SIZE, stride, stride,
                      LV_DISPLAY_ROTATION_270, CANVAS_COLOR_FORMAT);
}

void init_label_dsc(lv_draw_label_dsc_t *dsc, lv_color_t color, const lv_font_t *font) {
    lv_draw_label_dsc_init(dsc);
    dsc->color = color;
    dsc->font = font;
}

void init_rect_dsc(lv_draw_rect_dsc_t *dsc, lv_color_t color) {
    lv_draw_rect_dsc_init(dsc);
    dsc->bg_color = color;
    dsc->bg_opa = LV_OPA_COVER;
    dsc->border_width = 0;
    dsc->radius = 0;
}

/* LVGL v9 removed the lv_canvas_draw_* convenience wrappers used by an
 * earlier API version this file was originally written against - draw onto
 * a canvas now goes through a layer + the generic lv_draw_* functions. */

void canvas_draw_text(lv_obj_t *canvas, int x, int y, int w,
                      lv_draw_label_dsc_t *dsc, const char *txt) {
    dsc->text = txt;
    lv_area_t area = {.x1 = x, .y1 = y, .x2 = x + w - 1, .y2 = CANVAS_SIZE - 1};
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_draw_label(&layer, dsc, &area);
    lv_canvas_finish_layer(canvas, &layer);
}

void canvas_draw_rect(lv_obj_t *canvas, int x, int y, int w, int h,
                      lv_draw_rect_dsc_t *dsc) {
    lv_area_t area = {.x1 = x, .y1 = y, .x2 = x + w - 1, .y2 = y + h - 1};
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_draw_rect(&layer, dsc, &area);
    lv_canvas_finish_layer(canvas, &layer);
}

