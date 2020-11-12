# Milk Sensing Firmware

The firmware is written using zephyrproject, https://docs.zephyrproject.org/latest/boards/arm/nrf52dk_nrf52832/doc/index.html, with lightweight C++ templated constructs for managing lifecycle functionality.

## Build

To build the toolchain, we will use the cmake build system and the GNU ARM toolchain. We control our version of the code base with `git` and use `west` to acquire the rest of the sources for zephyr. Zephyr generates a `CMake`, build system generator, to build and uses `west` to manage the rest of the interactions with the boards, host tools, and `KConfig`.

1. Install the GNU ARM toolchain (and configure the shell environment)
2. Install `west`
3. Install `jlink` (SEGGER jlink tools)
4. Build `make build`
5. Connect a board and flash with `make flash`

## Test and Debug

The ztest infrastructure is not yet integrated withthe project. Instead, we have relied on manual testing and logging. To view logs connect the device via usb and use the path for a device in `/dev/` that looks something like the command: `screen /dev/tty.usbmodem0006829572021 115200`.

In another panel, run `make jlink-gdbserver` to start a gdb server connected to the device. Connect a gdb client to the gdb server in another panel with `make jlink-gdbclient`. This client will start up, read the symbols, reset to the beginning of boot, and wait for you to start.

* `mon reset 0`: Start from the beginning
* `c`: Continue (or start execution from current location)
* `n`: Step one command forward
* `s`: Step into the next command (goes into a function if next command is a function)
* `b asdf.cpp:34`: Set a breakpoint on the line that is one line 34 of file asdf.cpp
* `d 2`: Delete the second breakpoint
* `info locals`: Print all of the local variables
* `ctrl+c`: Interrupt a running program (follow up with `mon reset 0`, then `c` to start over without resetting breakpoints)


## OTA DFU

There are several options for doing OTA firmware upgrades. All of the wildcard matches for make rules are the device name that will be flashed/built.

The fastest way is to build locally, air drop to an iPhone, and use MCU Manager upload from nRF Toolbox.

The simplest way is to use the make dfu-% target, which matches on the desired device name.

### iPhone OTA DFU

0. Clean the build directory to just in case: `make clean`
1. Build a signed app locally: `make app-ASS0`
2. Air Drop to iPhone: `open build_app/zephyr` in Finder, then right click and use share over Air Drop for `zephyr.signed.bin`
3. On the iPhone accept the file in nRF Toolbox
4. Start the device advertising
5. Select the MCU Manager
6. Select the device name that is currently advertising
7. Select the latest shared zephyr.signed.bin file
8. Wait for success
9. Power cycle the device

### Laptop OTA DFU

If the device name will not change, then we don't have to separate steps

0. Clean the build directory just in case: `make clean`
1. Run `make dfu-ASS0`


If the device name will change, then we have to split the command to search for the device with the correct name. In the following commands MSD1 is the current name and ASS0 will be the new name.

0. Clean the build directory just in case: `make clean`
1. Build a signed app locally: `make app-ASS0`
2. Upload and boot into the new image `make dfu-upload-MSD1 dfu-test-MSD1`
3. Confirm successful upgrade: `make dfu-confirm-ASS0 dfu-reset-ASS0`

### OTA DFU Errors

Typically errors will be due to advertising the wrong name, check with nRF Connect if the app is advertising under an unexpected name. Power reset to return to the previous image if the tested image is not already confirmed.


