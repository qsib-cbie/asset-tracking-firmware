#ifndef APP_INCLUDE_APP_LOG_HPP
#define APP_INCLUDE_APP_LOG_HPP

// #define LOG_LEVEL LOG_LEVEL_DBG
#define LOG_LEVEL LOG_LEVEL_INF
#define LOG_MODULE_NAME ble_app
#include <logging/log.h>
#include <logging/log_ctrl.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#endif