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

void draw_battery(lv_obj_t *canvas, int x, int y, uint8_t level, bool charging) {
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_line_dsc_t line_dsc;
    init_rect_dsc(&rect_dsc, LVGL_FOREGROUND);
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    /* outer shell: 22×10 */
    rect_dsc.bg_opa = LV_OPA_TRANSP;
    rect_dsc.border_width = 1;
    rect_dsc.border_color = LVGL_FOREGROUND;
    canvas_draw_rect(canvas, x, y, 22, 10, &rect_dsc);

    /* nub */
    rect_dsc.border_width = 0;
    rect_dsc.bg_opa = LV_OPA_COVER;
    canvas_draw_rect(canvas, x + 22, y + 3, 2, 4, &rect_dsc);

    /* fill */
    uint8_t fill_w = (level * 20) / 100;
    if (fill_w > 0) {
        canvas_draw_rect(canvas, x + 1, y + 1, fill_w, 8, &rect_dsc);
    }

    /* charging bolt — simple V shape */
    if (charging) {
        lv_point_t pts[] = {{x + 12, y + 2}, {x + 10, y + 5}, {x + 12, y + 5}, {x + 10, y + 8}};
        canvas_draw_line(canvas, pts, 4, &line_dsc);
    }
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

void init_line_dsc(lv_draw_line_dsc_t *dsc, lv_color_t color, uint8_t width) {
    lv_draw_line_dsc_init(dsc);
    dsc->color = color;
    dsc->width = width;
}

void canvas_draw_text(lv_obj_t *canvas, int x, int y, int w,
                      lv_draw_label_dsc_t *dsc, const char *txt) {
    lv_canvas_draw_text(canvas, x, y, w, dsc, txt);
}

void canvas_draw_rect(lv_obj_t *canvas, int x, int y, int w, int h,
                      lv_draw_rect_dsc_t *dsc) {
    lv_area_t area = {.x1 = x, .y1 = y, .x2 = x + w - 1, .y2 = y + h - 1};
    lv_canvas_draw_rect(canvas, area.x1, area.y1, w, h, dsc);
}

void canvas_draw_line(lv_obj_t *canvas, lv_point_t points[], uint16_t count,
                      lv_draw_line_dsc_t *dsc) {
    for (uint16_t i = 0; i + 1 < count; i++) {
        lv_canvas_draw_line(canvas, &points[i], &points[i + 1], dsc);
    }
}

void canvas_draw_img(lv_obj_t *canvas, int x, int y, const void *src,
                     lv_draw_image_dsc_t *dsc) {
    lv_canvas_draw_image(canvas, x, y, src, dsc);
}

void draw_circle(lv_obj_t *canvas, int x, int y, int size, bool filled) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    if (filled) {
        dsc.bg_color   = LVGL_FOREGROUND;
        dsc.bg_opa     = LV_OPA_COVER;
        dsc.border_width = 0;
    } else {
        dsc.bg_opa       = LV_OPA_TRANSP;
        dsc.border_color = LVGL_FOREGROUND;
        dsc.border_opa   = LV_OPA_COVER;
        dsc.border_width = 1;
    }
    canvas_draw_rect(canvas, x, y, size, size, &dsc);
}

void draw_glyph(lv_obj_t *canvas, int x, int y,
                const lv_image_dsc_t *glyph, bool active) {
    lv_draw_image_dsc_t img_dsc;
    lv_draw_image_dsc_init(&img_dsc);
    img_dsc.recolor     = active ? LVGL_FOREGROUND : lv_color_make(0xAA, 0xAA, 0xAA);
    img_dsc.recolor_opa = active ? LV_OPA_COVER : LV_OPA_50;
    canvas_draw_img(canvas, x, y, glyph, &img_dsc);
}
