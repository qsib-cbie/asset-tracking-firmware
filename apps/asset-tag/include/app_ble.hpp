#ifndef APP_INCLUDE_APP_BLE_HPP
#define APP_INCLUDE_APP_BLE_HPP

#include <app_log.hpp>
#ifdef CONFIG_MCUMGR_SMP_BT
#include <app_smp.hpp>
#endif

#include <app/ass.hpp>

#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>

#include <stdexcept>

namespace app_ble {

#ifdef CONFIG_MCUMGR_SMP_BT
    static constexpr bt_data advertisement_data[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0f, 0x18),
        BT_DATA_BYTES(BT_DATA_UUID128_SOME, BT_UUID_ASS_DATA_BYTES)
    };

    static const bt_data scan_response_data[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_SOME, BT_UUID_SMP_DATA_BYTES)
    };
#else
    static constexpr bt_data advertisement_data[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0f, 0x18),
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_ASS_DATA_BYTES)
    };
#endif

    static bt_le_adv_param adv_params[] = {
        BT_LE_ADV_PARAM_INIT(
            BT_LE_ADV_OPT_CONNECTABLE |  BT_LE_ADV_OPT_USE_NAME,
            BT_GAP_ADV_FAST_INT_MIN_2,
            BT_GAP_ADV_FAST_INT_MAX_2,
            NULL)
        };

    struct static_manager_t {
        static void bt_adv_start() {
            bt_le_adv_stop();
            bt_set_name(ass_data);
#ifdef CONFIG_MCUMGR_SMP_BT
            const auto err = bt_le_adv_start(
                adv_params,
                advertisement_data,
                ARRAY_SIZE(advertisement_data),
                scan_response_data,
                ARRAY_SIZE(scan_response_data));
#else
            const auto err = bt_le_adv_start(
                adv_params,
                advertisement_data,
                ARRAY_SIZE(advertisement_data),
                NULL,
                0);
#endif
            if (err) {
                LOG_ERR("Advertising failed to start (err %d)", err);
                throw std::runtime_error("Failed to start advertising");
            }

            LOG_DBG("Advertising successfully started");
        }

        static void bt_adv_stop() {
            bt_le_adv_stop();
        }

        static void connected(bt_conn *conn, uint8_t err) {
            if (err) {
                LOG_INF("Connection failed (err 0x%02x)", err);
            } else {
                LOG_INF("Connected");
            }
        }

        static void disconnected(bt_conn *conn, uint8_t reason) {
            LOG_INF("Disconnected (reason 0x%02x)", reason);
        }
    };

    static bt_conn_cb conn_callbacks = {
        .connected = static_manager_t::connected,
        .disconnected = static_manager_t::disconnected,
    };

    struct manager_t {
        manager_t() {
            int ret;

            // Register the built-in mcumgr command handlers before advertising */
            #ifdef CONFIG_MCUMGR_CMD_OS_MGMT
                os_mgmt_register_group();
            #endif
            #ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
                img_mgmt_register_group();
            #endif

            // Prepare kernel structures
            ret = bt_enable(NULL);
            if(ret) {
                LOG_ERR("Failed to enable bluetooth: %d", ret);
                throw std::runtime_error("Failed to enable bluetooth");
            }
            LOG_DBG("Bluetooth initialized");

            // Register advertisement and callback configurations
            bt_conn_cb_register(&conn_callbacks);
            #ifdef CONFIG_MCUMGR_SMP_BT
            smp_bt_register();
            #endif
        }

        void start() {
            static_manager_t::bt_adv_start();
        }

        void stop() {
            static_manager_t::bt_adv_stop();
        }
    };
}


#endif