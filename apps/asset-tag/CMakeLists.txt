# SPDX-License-Identifier: Apache-2.0

IF(DEFINED ENV{MOCK_DATA})
    add_definitions(-DMOCK_DATA)
ENDIF()

cmake_minimum_required(VERSION 3.13.1)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_app)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(AFTER include)
FILE(GLOB app_sources src/*.c src/*.cpp)
target_sources(app PRIVATE
  ${app_sources}
  )
