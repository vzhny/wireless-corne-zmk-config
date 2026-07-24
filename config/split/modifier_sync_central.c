#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include "split/modifier_sync.h"
#include "widgets/blecorne_central.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct bt_conn *periph_conn;
static uint16_t mod_char_handle;

static struct bt_gatt_discover_params discover_params;
static struct bt_uuid_128 svc_uuid  = BT_UUID_INIT_128(BT_UUID_BLECORNE_MOD_SVC_VAL);
static struct bt_uuid_128 char_uuid = BT_UUID_INIT_128(BT_UUID_BLECORNE_MOD_CHAR_VAL);

/* ── GATT discovery ──────────────────────────────────────────────────── */

static uint8_t discover_cb(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params) {
    if (!attr) {
        return BT_GATT_ITER_STOP;
    }
    if (params->type == BT_GATT_DISCOVER_PRIMARY) {
        const struct bt_gatt_service_val *svc = attr->user_data;
        discover_params.uuid        = &char_uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.end_handle   = svc->end_handle;
        discover_params.type         = BT_GATT_DISCOVER_CHARACTERISTIC;
        bt_gatt_discover(conn, &discover_params);
        return BT_GATT_ITER_STOP;
    }
    if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
        mod_char_handle = bt_gatt_attr_value_handle(attr);
        return BT_GATT_ITER_STOP;
    }
    return BT_GATT_ITER_STOP;
}

static void start_discovery(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(discover_work, start_discovery);

static void start_discovery(struct k_work *work) {
    if (!periph_conn) {
        return;
    }
    discover_params.uuid         = &svc_uuid.uuid;
    discover_params.func         = discover_cb;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle   = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type         = BT_GATT_DISCOVER_PRIMARY;
    int err = bt_gatt_discover(periph_conn, &discover_params);
    if (err == -EBUSY) {
        /* ZMK split is still doing its own GATT ops — retry in 500 ms */
        k_work_reschedule(&discover_work, K_MSEC(500));
    } else if (err) {
        LOG_WRN("modifier_sync: GATT discover failed: %d", err);
    }
}

/* ── BLE connection callbacks ────────────────────────────────────────── */

static void on_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        return;
    }
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info)) {
        return;
    }
    /* We only care about connections where this device is the BLE central
     * (i.e., the connection to the right-half peripheral). */
    if (info.role != BT_CONN_ROLE_CENTRAL) {
        return;
    }
    periph_conn = bt_conn_ref(conn);
    /* Delay to let ZMK's split protocol finish its own GATT discovery first */
    k_work_reschedule(&discover_work, K_MSEC(1000));
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason) {
    if (conn != periph_conn) {
        return;
    }
    bt_conn_unref(periph_conn);
    periph_conn = NULL;
    mod_char_handle = 0;
}

BT_CONN_CB_DEFINE(mod_sync_conn_cb) = {
    .connected    = on_connected,
    .disconnected = on_disconnected,
};

/* ── Modifier state forwarding ───────────────────────────────────────── */

static void send_mod_state(void) {
    if (!periph_conn || !mod_char_handle) {
        return;
    }
    /* The central widget's display-only shadow-tracked mods (see
     * blecorne_central.c) - the peripheral's mod cells get the same
     * real-time approximation the left half's own display does. */
    uint8_t full_mods = blecorne_central_get_display_mods();
    /* Extract right-side modifier bits (bits 4-7) into a nibble (bits 0-3) */
    uint8_t r_mods = (full_mods >> 4) & 0x0F;
    uint8_t active_layer = zmk_keymap_highest_layer_active();
    bool is_mac     = (active_layer == 1 || active_layer == 3);
    bool is_colemak = (active_layer == 2 || active_layer == 3);
    /* Bit 4: Mac/Win glyph-order flag. Bit 5: Qwerty/Colemak flag (drives
     * the peripheral's layout-name row). Peripheral has no local keymap/
     * layer state of its own to derive either from. */
    uint8_t payload = r_mods | (is_mac ? BIT(4) : 0) | (is_colemak ? BIT(5) : 0);
    bt_gatt_write_without_response(periph_conn, mod_char_handle, &payload, 1, false);
}

void modifier_sync_notify_mods_changed(void) {
    send_mod_state();
}

static int keycode_event_cb(const zmk_event_t *eh) {
    send_mod_state();
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_event_cb(const zmk_event_t *eh) {
    send_mod_state();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(mod_sync_keys, keycode_event_cb);
ZMK_SUBSCRIPTION(mod_sync_keys, zmk_keycode_state_changed);

ZMK_LISTENER(mod_sync_layer, layer_event_cb);
ZMK_SUBSCRIPTION(mod_sync_layer, zmk_layer_state_changed);
