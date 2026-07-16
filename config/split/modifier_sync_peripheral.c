#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "split/modifier_sync.h"
#include "widgets/blecorne_peripheral.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct bt_uuid_128 mod_svc_uuid  = BT_UUID_INIT_128(BT_UUID_BLECORNE_MOD_SVC_VAL);
static struct bt_uuid_128 mod_char_uuid = BT_UUID_INIT_128(BT_UUID_BLECORNE_MOD_CHAR_VAL);

static ssize_t write_mod_state(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len,
                                uint16_t offset, uint8_t flags) {
    if (len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    blecorne_peripheral_update_mods(*(const uint8_t *)buf);
    return len;
}

BT_GATT_SERVICE_DEFINE(blecorne_mod_svc,
    BT_GATT_PRIMARY_SERVICE(&mod_svc_uuid),
    BT_GATT_CHARACTERISTIC(&mod_char_uuid,
                           BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_mod_state, NULL),
);
