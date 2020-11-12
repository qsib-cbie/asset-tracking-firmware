SHELL=/bin/bash

BOARD                  := nrf52dk_nrf52832
BOARD_ROOT             := $(abspath ./zephyr)

BOOTLOADER_BUILD_DIR   := mcuboot
BOOTLOADER_SRC_DIR     := bootloader/mcuboot/boot/zephyr
BOOTLOADER_KEY         := bootloader/mcuboot/root-rsa-2048.pem

APP_BUILD_DIR          := app
APP_SRC_DIR            := apps/asset-tag

ifdef SAMPLES_SYSTEM_OFF
APP_SRC_DIR            := zephyr/samples/boards/nrf/system_off

endif

ifdef SAMPLES_SMP
APP_SRC_DIR            := zephyr/samples/subsys/mgmt/mcumgr/smp_svr

# Enable if using non default APP_SRC_DIR, Invalidates build cache each time
EXTRA_BUILD_OPTS       := -- -DBOARD_ROOT=${BOARD_ROOT} -DBOARD=${BOARD}
EXTRA_BUILD_OPTS       += -DOVERLAY_CONFIG=overlay-bt.conf
endif

RUNNER                 ?= nrfjprog

HEX_PATH               := $(abspath build_${APP_BUILD_DIR}/zephyr/zephyr.signed.hex)
BIN_PATH               := $(abspath build_${APP_BUILD_DIR}/zephyr/zephyr.signed.bin)
UNSIGNED_HEX_PATH      := $(abspath build_${APP_BUILD_DIR}/zephyr/zephyr.hex)
UNSIGNED_BIN_PATH      := $(abspath build_${APP_BUILD_DIR}/zephyr/zephyr.bin)

.PHONY: build
build: bootloader app
	@echo Done!
	@echo "hex: ${HEX_PATH}"
	@echo "bin: ${BIN_PATH}"

.PRECIOUS: build_${APP_BUILD_DIR}
build_${APP_BUILD_DIR}:
	mkdir -p build_${APP_BUILD_DIR}

.PRECIOUS: build_${APP_BUILD_DIR}/overlay-device-%.conf
build_${APP_BUILD_DIR}/overlay-device-%.conf: build_${APP_BUILD_DIR}
	@ls build_${APP_BUILD_DIR} | grep 'overlay-device-.*.conf' | grep -v overlay-device-$*.conf | xargs -I{} rm build_${APP_BUILD_DIR}/{} || true
	@[[ -f $@ ]] || (echo "Writing $@" && echo 'CONFIG_BT_DEVICE_NAME="'$*'"' > $@)

.PHONY: flash-%
flash-%: bootloader_flash | app_flash-%
	@echo Done!
	@echo "hex: ${HEX_PATH}"
	@echo "bin: ${BIN_PATH}"

.PHONY: guiconfig
guiconfig:
	west build -t guiconfig -p auto -d build_${APP_BUILD_DIR} -b ${BOARD} ${APP_SRC_DIR}

.PHONY: bootloader
bootloader:
	west build -p auto -d build_${BOOTLOADER_BUILD_DIR} -b ${BOARD} ${BOOTLOADER_SRC_DIR} -- -DBOARD_ROOT=${BOARD_ROOT}

.PHONY: bootloader_flash
bootloader_flash: bootloader
	west flash --runner ${RUNNER} --build-dir build_${BOOTLOADER_BUILD_DIR}

.PHONY: debug_app-%
debug_app-%: build_${APP_BUILD_DIR}/overlay-device-%.conf
	west build -p auto -d build_${APP_BUILD_DIR} -b ${BOARD} ${APP_SRC_DIR} -- -DOVERLAY_CONFIG=overlay-debug.conf

.PHONY: debug_app_flash-%
debug_app_flash-%: debug_app-%
	west flash --runner ${RUNNER} --build-dir build_${APP_BUILD_DIR} --hex-file ${UNSIGNED_HEX_PATH} --bin-file ${UNSIGNED_BIN_PATH}

.PHONY: app-%
app-%: build_${APP_BUILD_DIR}/overlay-device-%.conf
	west build -p auto -d build_${APP_BUILD_DIR} --board=${BOARD} ${APP_SRC_DIR} ${EXTRA_BUILD_OPTS}
	west sign -t imgtool -d build_${APP_BUILD_DIR} -- --key ${BOOTLOADER_KEY}

.PHONY: app_flash-%
app_flash-%: app-%
	west flash --runner ${RUNNER} --build-dir build_${APP_BUILD_DIR} --hex-file ${HEX_PATH} --bin-file ${BIN_PATH}

pyocd-gdbserver:
	pyocd gdbserver

pyocd-gdbclient:
	${HOME}/opt/gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi-gdb -x .pyocd-gdbinit build_${APP_BUILD_DIR}/zephyr/zephyr.elf

jlink-gdbserver:
	JLinkGDBServer -Device NRF52 -If SWD -Speed 4000 -RTTChannel 0

jlink-gdbclient:
	${HOME}/opt/gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi-gdb -x .jlink-gdbinit build_${APP_BUILD_DIR}/zephyr/zephyr.elf

logs:
	JLinkRTTClient

.PHONY: dfu-list-%
dfu-list-%:
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list

.PHONY: dfu-upload-%
dfu-upload-%:
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list
	@newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list
	@sleep 1
	@echo "Uploading image ... This may take a while"
	@newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image upload ${BIN_PATH}
	@sleep 5
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list | tee build_${APP_BUILD_DIR}/list_after_upload.txt
	@sleep 1
	cat build_${APP_BUILD_DIR}/list_after_upload.txt | grep hash | awk '{print $$2}' | sed -n 2p | tee build_${APP_BUILD_DIR}/new_image_hash.txt

.PHONY: dfu-test-%
dfu-test-%:
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image test $(shell cat build_${APP_BUILD_DIR}/new_image_hash.txt)
	@echo "Resetting to test image ..."
	@sleep 1
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" reset
	@echo "Booting ..."
	@sleep 10

.PHONY: dfu-confirm-%
dfu-confirm-%:
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list
	@echo "Confirming booted image"
	@sleep 1
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image confirm
	@sleep 1

.PHONY: dfu-reset-%
dfu-reset-%:
	@echo "Resetting to confirmed image ..."
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" reset
	@echo "Booting ..."
	@sleep 10
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list
	@sleep 1

.PHONY: dfu-old-hash-%
dfu-old-hash-%:
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list | grep hash | awk '{print $$2}' | sed -n 1p

.PHONY: dfu-new-hash-%
dfu-new-hash-%:
	newtmgr --conntype ble --connstring ctlr_name=hci0,peer_name="$*" image list | grep hash | awk '{print $$2}' | sed -n 2p

.PHONY: dfu-%
dfu-%: app-% dfu-upload-% dfu-test-% dfu-confirm-% dfu-reset-%
	@echo "Done booting into confirmed image"

.PHONY: clean
clean:
	rm -rf build*
