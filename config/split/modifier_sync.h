#pragma once

#include <zephyr/bluetooth/uuid.h>

/*
 * Custom BLE GATT service: central writes R-modifier state to peripheral.
 *
 * r_mods byte encoding (matches HID mod byte >> 4):
 *   bit 0 = RCtrl
 *   bit 1 = RShift
 *   bit 2 = RAlt
 *   bit 3 = RGUI
 */

#define BT_UUID_BLECORNE_MOD_SVC_VAL \
    BT_UUID_128_ENCODE(0x1BC5D5A0, 0x0001, 0x4BE5, 0xBCC3, 0x000000000000ULL)

#define BT_UUID_BLECORNE_MOD_CHAR_VAL \
    BT_UUID_128_ENCODE(0x1BC5D5A0, 0x0001, 0x4BE5, 0xBCC3, 0x000000000001ULL)
