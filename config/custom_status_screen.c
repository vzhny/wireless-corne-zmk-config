#include <zephyr/kernel.h>
#include <lvgl.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
#include "widgets/blecorne_central.h"
static struct blecorne_central_widget central_widget;
#else
#include "widgets/blecorne_peripheral.h"
static struct blecorne_peripheral_widget peripheral_widget;
#endif

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
    blecorne_central_widget_init(&central_widget, screen);
    lv_obj_align(blecorne_central_widget_obj(&central_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#else
    blecorne_peripheral_widget_init(&peripheral_widget, screen);
    lv_obj_align(blecorne_peripheral_widget_obj(&peripheral_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    return screen;
}
