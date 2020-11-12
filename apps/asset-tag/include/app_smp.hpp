#ifndef APP_INCLUDE_APP_SMP_HPP
#define APP_INCLUDE_APP_SMP_HPP

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <stats/stats.h>
#include <mgmt/mcumgr/buf.h>

#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <mgmt/mcumgr/smp_bt.h>
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
#define BT_UUID_SMP_DATA_BYTES 0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86, 0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d
#endif



#endif