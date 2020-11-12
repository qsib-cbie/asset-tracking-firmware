#ifndef APP_INCLUDE_APP_ASS_HPP
#define APP_INCLUDE_APP_ASS_HPP

#include <stdbool.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <app/version.hpp>

// 96f062c4-b99e-4141-9439-c4f9db977899
#define BT_UUID_ASS_DATA_BYTES 0x99, 0x78, 0x97, 0xdb, 0xf9, 0xc4, 0x39, 0x94, 0x41, 0x41, 0x9e, 0xb9, 0xc4, 0x62, 0xf0, 0x96
#define BT_UUID_ASS BT_UUID_DECLARE_128(BT_UUID_ASS_DATA_BYTES)

// 856d27bf-b8e1-4109-bf61-5733f4e1b299
#define BT_UUID_ASS_VALUE BT_UUID_DECLARE_128(0x99, 0xb2, 0xe1, 0xf4, 0x33, 0x57, 0x61, 0xbf, 0x09, 0x41, 0xe1, 0xb8, 0xbf, 0x27, 0x6d, 0x85)

// 08aefe30-27c5-4d34-9936-d4cc6188ee99
#define BT_UUID_ASS_ERROR BT_UUID_DECLARE_128(0x99, 0xee, 0x88, 0x61, 0xcc, 0xd4, 0x36, 0x99, 0x34, 0x4d, 0xc5, 0x27, 0x30, 0xfe, 0xae, 0x08)

// 4cc69818-27c5-4d34-9936-d4cc6188ee99
#define BT_UUID_ASS_VERSION BT_UUID_DECLARE_128(0x99, 0xee, 0x88, 0x61, 0xcc, 0xd4, 0x36, 0x99, 0x34, 0x4d, 0xc5, 0x27, 0x18, 0x98, 0xc6, 0x4c)

// 9787a554-76cc-4d02-99bb-aa7d5a4f4a99
#define BT_UUID_ASS_DATA BT_UUID_DECLARE_128(0x99, 0x4a, 0x4f, 0x5a, 0x7d, 0xaa, 0xbb, 0x99, 0x02, 0x4d, 0xcc, 0x76, 0x54, 0xa5, 0x87, 0x97)

/** @brief Notify temperature of NTC that may be heated.
 *
 * This will send a GATT notification to all current subscribers.
 *
 * @param temp0_celcius The degrees Celcius the NTC, accurate to at least 3 decimal places
 *
 * @return Zero in case of success and error code in case of error
 */
int ass_temp0_notify(float temp0_celcius);


static uint8_t ass_updated = 0;
static char ass_value[128] = {0};
static char ass_version[sizeof(VERSION)] = {0};
static char ass_error[128] = {0};
static char ass_data[128] = {0};

// Readable Characteristic Handlers

static ssize_t read_ass_value(bt_conn* conn, const bt_gatt_attr* attr, void* buf, uint16_t len, uint16_t offset) {
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, strlen(ass_value));
}

static ssize_t read_ass_version(bt_conn* conn, const bt_gatt_attr* attr, void* buf, uint16_t len, uint16_t offset) {
	memcpy(ass_version, VERSION, sizeof(VERSION));
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, strlen(ass_version));
}

static ssize_t read_ass_error(bt_conn* conn, const bt_gatt_attr* attr, void* buf, uint16_t len, uint16_t offset) {
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, strlen(ass_error));
}

static ssize_t read_ass_data(bt_conn* conn, const bt_gatt_attr* attr, void* buf, uint16_t len, uint16_t offset) {
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, strlen(ass_data));
}

// Writable Characteristic Handlers

static ssize_t write_ass_value(bt_conn* conn, const bt_gatt_attr* attr, const void* buf, uint16_t len, uint16_t offset, uint8_t flags) {
	uint8_t* value = reinterpret_cast<uint8_t*>(attr->user_data);
	if(offset + len > sizeof(ass_value)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	ass_updated |= BIT(0);
	LOG_INF("Wrote ass_value(%d, %d), ass_updated: 0x%01x", (int) offset, (int) len, (int) ass_updated);

	return len;
}

static ssize_t write_ass_data(bt_conn* conn, const bt_gatt_attr* attr, const void* buf, uint16_t len, uint16_t offset, uint8_t flags) {
	uint8_t* value = reinterpret_cast<uint8_t*>(attr->user_data);
	if(offset + len > sizeof(ass_data)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	ass_updated |= BIT(1);
	LOG_INF("Wrote ass_data(%d, %d), ass_updated: 0x%01x", (int) offset, (int) len, (int) ass_updated);

	return len;
}

// GATT uses ATT, so the attrs index has other entries than the high level macros
// Read this guide and use gdb for more details
// https://www.novelbits.io/bluetooth-gatt-services-characteristics/

BT_GATT_SERVICE_DEFINE(ass_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ASS),
    BT_GATT_CHARACTERISTIC(BT_UUID_ASS_VALUE,
            BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
            BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
            read_ass_value, write_ass_value, ass_value),
	BT_GATT_CHARACTERISTIC(BT_UUID_ASS_ERROR,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_ass_error, NULL, ass_error),
    BT_GATT_CHARACTERISTIC(BT_UUID_ASS_VERSION,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_ass_version, NULL, ass_version),
	BT_GATT_CHARACTERISTIC(BT_UUID_ASS_DATA,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_ass_data, write_ass_data, ass_data),
);

static int ass_init(const device* dev) {
	ARG_UNUSED(dev);

	memset(ass_value, 0, sizeof(ass_value));
	memset(ass_data, 0, sizeof(ass_data));
	ass_updated = 0;

	return 0;
}

int ass_value_write(std::string_view data) {
	size_t message_length = std::min(size_t{data.size()}, size_t{sizeof(ass_value) - 1});
	memcpy(ass_value, data.data(), message_length);
	ass_value[message_length] = '\0';
	LOG_INF("Copied %d bytes of message to ass_value", message_length);
	LOG_INF("ass_value: %s", log_strdup(reinterpret_cast<char*>(ass_value)));
	return 0;
}

int ass_error_write(std::string_view data) {
	size_t message_length = std::min(size_t{data.size()}, size_t{sizeof(ass_error) - 1});
	memcpy(ass_error, data.data(), message_length);
	ass_error[message_length] = '\0';
	LOG_INF("Copied %d bytes of message to ass_error", message_length);
	LOG_INF("ass_error: %s", log_strdup(reinterpret_cast<char*>(ass_error)));
	return 0;
}

int ass_data_write(std::string_view data) {
	size_t message_length = std::min(size_t{data.size()}, size_t{sizeof(ass_data) - 1});
	memcpy(ass_data, data.data(), message_length);
	ass_data[message_length] = '\0';
	LOG_INF("Copied %d bytes of message to ass_data", message_length);
	LOG_INF("ass_data: %s", log_strdup(reinterpret_cast<char*>(ass_data)));
	return 0;
}

SYS_INIT(ass_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
