#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

/*
 * Boot-time LED flash: blink blue_led (P1.09) 5 times during firmware
 * initialization, then leave it off. Runs at APPLICATION priority 5
 * so it fires before ZMK subsystems start and before any background
 * threads touch the LED.
 *
 * Note: the LED turning on while USB-charging is NOT controlled by
 * this file — see charging_led.md for that explanation.
 */

static const struct gpio_dt_spec boot_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

static int boot_led_flash(void)
{
    if (!gpio_is_ready_dt(&boot_led)) {
        return 0;
    }
    gpio_pin_configure_dt(&boot_led, GPIO_OUTPUT_INACTIVE);

    for (int i = 0; i < 5; i++) {
        gpio_pin_set_dt(&boot_led, 1);
        k_sleep(K_MSEC(80));
        gpio_pin_set_dt(&boot_led, 0);
        k_sleep(K_MSEC(80));
    }

    return 0;
}

SYS_INIT(boot_led_flash, APPLICATION, 5);
