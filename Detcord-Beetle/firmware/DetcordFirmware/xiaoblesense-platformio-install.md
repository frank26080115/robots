The Roach project, and the Detcord robot, switched from using the Adafruit ItsyBitsy to using the Seeed Studio XIAO BLE Sense

There's no official support for the XIAO BLE Sense with PlatformIO at this time.

This is instructions for setting up the XIAO BLE Sense with PlatformIO. There are existing instructions available but the method here will use the core/framework that:

 * compatible with Adafruit libraries
   * specifically important: TinyUSB, LittleFS
 * does not use Mbed OS

## Editing platformio.ini

    #board = adafruit_itsybitsy_nrf52840
    board = xiaoblesense

## New file: xiaoblesense.json

Path example: `C:\Users\frank\.platformio\platforms\nordicnrf52\boards\xiaoblesense.json`

File contents:

    {
        "build": {
            "arduino":{
                "ldscript": "nrf52840_s140_v7.ld"
            },
            "core": "nRF5",
            "cpu": "cortex-m4",
            "extra_flags": "-DSEEED_XIAO_NRF52840_SENSE -DNRF52840_XXAA",
            "f_cpu": "64000000L",
            "hwids": [[ "0x2886", "0x0045" ], [ "0x2886", "0x8045" ]],
            "mcu": "nrf52840",
            "variant": "Seeed_XIAO_nRF52840_Sense",
            "bsp": { "name": "xiaoblesense" },
            "softdevice": {
                "sd_flags": "-DS140",
                "sd_name": "s140",
                "sd_version": "7.3.0",
                "sd_fwid": "0x0123"
            },
            "bootloader": { "settings_addr": "0xFF000" }
        },
        "connectivity": [
            "bluetooth"
        ],
        "debug": {
            "jlink_device": "nRF52840_xxAA",
            "openocd_target": "nrf52.cfg",
            "svd_path": "nrf52840.svd"
        },
        "frameworks": [
            "arduino"
        ],
        "name": "Seeed XIAO BLE Sense",
        "upload": {
            "maximum_ram_size": 237568,
            "maximum_size": 811008,
            "speed": 115200,
            "protocol": "nrfutil",
            "protocols": [
                "jlink",
                "nrfjprog",
                "nrfutil",
                "stlink",
                "cmsis-dap",
                "blackmagic"
            ],
            "use_1200bps_touch": true,
            "require_upload_port": true,
            "wait_for_upload_port": true
        },
        "url": "https://wiki.seeedstudio.com/XIAO_BLE",
        "vendor": "Seeed Studio"
    }

## Edit file: platform.py

Path example: `C:\Users\frank\.platformio\platforms\nordicnrf52\platform.py`

Edit the line that looks like:

    if self.board_config(board).get("build.bsp.name", "nrf5") == "adafruit":

to look like:

    if self.board_config(board).get("build.bsp.name", "nrf5") == "adafruit" or self.board_config(board).get("build.bsp.name", "nrf5") == "xiaoblesense":

## Edit file: arduino.py

Path example: `C:\Users\frank\.platformio\platforms\nordicnrf52\builder\frameworks\arduino.py`

Part of original file:

    if board.get("build.bsp.name", "nrf5") == "adafruit":
        env.SConscript("arduino/adafruit.py")
    else:
        env.SConscript("arduino/nrf5.py")

edit that part to look like:

    if board.get("build.bsp.name", "nrf5") == "adafruit":
        env.SConscript("arduino/adafruit.py")
    elif board.get("build.bsp.name", "nrf5") == "xiaoblesense":
        env.SConscript("arduino/adafruit.py")
    else:
        env.SConscript("arduino/nrf5.py")

Reason: `nrf5.py` forces the usage of `framework-arduinonordicnrf5` but we need to be using `framework-arduinoadafruitnrf52`

## Install SoftDevice SDK

The XIAO BLE Sense uses a different SoftDevice SDK

Download from: https://www.nordicsemi.com/Products/Development-software/S140/Download version `S140 v7.3.0`

File should be an archive, extract it, and notice the directory `s140_nrf52_7.3.0_API`

Install it as (example path): `C:\Users\frank\.platformio\packages\framework-arduinoadafruitnrf52\cores\nRF5\nordic\softdevice\s140_nrf52_7.3.0_API`

## Install variant: XIAO BLE Sense

Download file `nrf52-1.1.1.tar.bz2` from `https://files.seeedstudio.com/arduino/core/nRF52/nrf52-1.1.1.tar.bz2`

note: this path was found by reading `https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json` , there may be updates

Extract the contents, notice the `variants` directory, containing `Seeed_XIAO_nRF52840` and `Seeed_XIAO_nRF52840_Sense`

Place the variants into (example path): `C:\Users\frank\.platformio\packages\framework-arduinoadafruitnrf52\variants`

Such that these exists:

 * `.....\framework-arduinoadafruitnrf52\variants\Seeed_XIAO_nRF52840`
 * `.....\framework-arduinoadafruitnrf52\variants\Seeed_XIAO_nRF52840_Sense`

## Install new linker script: nrf52840_s140_v7.ld

In the extracted contents of `nrf52-1.1.1.tar.bz2`, look for `....\nrf52-1.1.1.tar\Adafruit_nRF52_Arduino-1.1.1\cores\nRF5\linker\nrf52840_s140_v7.ld`

Copy to `....\framework-arduinoadafruitnrf52\cores\nRF5\linker\nrf52840_s140_v7.ld`

## Edit: boards.txt

Path example: `C:\Users\frank\.platformio\packages\framework-arduinoadafruitnrf52\boards.txt`

Insert this content somewhere:

    # -----------------------------------
    # XIAO BLE Sense
    # -----------------------------------
    xiaoblesense.name=Seeeduino XIAO BLE Sense

    # VID/PID for Bootloader, Arduino & CircuitPython
    xiaoblesense.vid.0=0x2886
    xiaoblesense.pid.0=0x0045
    xiaoblesense.vid.1=0x2886
    xiaoblesense.pid.1=0x8045

    # Upload
    xiaoblesense.bootloader.tool=bootburn
    xiaoblesense.upload.tool=nrfutil
    xiaoblesense.upload.protocol=nrfutil
    xiaoblesense.upload.use_1200bps_touch=true
    xiaoblesense.upload.wait_for_upload_port=true
    xiaoblesense.upload.maximum_size=811008
    xiaoblesense.upload.maximum_data_size=237568

    # Build
    xiaoblesense.build.mcu=cortex-m4
    xiaoblesense.build.f_cpu=64000000
    xiaoblesense.build.board=SEEED_XIAO_NRF52840_SENSE -DSEEED_XIAO_NRF52840_SENSE
    xiaoblesense.build.core=nRF5
    xiaoblesense.build.variant=Seeed_XIAO_nRF52840_Sense
    xiaoblesense.build.usb_manufacturer="Seeed Studio"
    xiaoblesense.build.usb_product="Seeed XIAO BLE Sense"
    xiaoblesense.build.extra_flags=-DNRF52840_XXAA {build.flags.usb}
    xiaoblesense.build.ldscript=nrf52840_s140_v6.ld
    xiaoblesense.build.vid=0x239A
    xiaoblesense.build.pid=0x8051

    # SoftDevice Menu
    xiaoblesense.menu.softdevice.s140v6=S140 7.3.0
    xiaoblesense.menu.softdevice.s140v6.build.sd_name=s140
    xiaoblesense.menu.softdevice.s140v6.build.sd_version=7.3.0
    xiaoblesense.menu.softdevice.s140v6.build.sd_fwid=0x0123

    # Debug Menu
    xiaoblesense.menu.debug.l0=Level 0 (Release)
    xiaoblesense.menu.debug.l0.build.debug_flags=-DCFG_DEBUG=0
    xiaoblesense.menu.debug.l1=Level 1 (Error Message)
    xiaoblesense.menu.debug.l1.build.debug_flags=-DCFG_DEBUG=1
    xiaoblesense.menu.debug.l2=Level 2 (Full Debug)
    xiaoblesense.menu.debug.l2.build.debug_flags=-DCFG_DEBUG=2
    xiaoblesense.menu.debug.l3=Level 3 (Segger SystemView)
    xiaoblesense.menu.debug.l3.build.debug_flags=-DCFG_DEBUG=3
    xiaoblesense.menu.debug.l3.build.sysview_flags=-DCFG_SYSVIEW=1

    # Debug Output Menu
    xiaoblesense.menu.debug_output.serial=Serial
    xiaoblesense.menu.debug_output.serial.build.logger_flags=-DCFG_LOGGER=0
    xiaoblesense.menu.debug_output.serial1=Serial1
    xiaoblesense.menu.debug_output.serial1.build.logger_flags=-DCFG_LOGGER=1 -DCFG_TUSB_DEBUG=CFG_DEBUG
    xiaoblesense.menu.debug_output.rtt=Segger RTT
    xiaoblesense.menu.debug_output.rtt.build.logger_flags=-DCFG_LOGGER=2 -DCFG_TUSB_DEBUG=CFG_DEBUG -DSEGGER_RTT_MODE_DEFAULT=SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL

## Adding QSPI flash IC to Adafruit_SPIFlash library

The flash IC used on the XIAO is not supported by `Adafruit_SPIFlash`, but it can be added easily

The definition of the flash IC is

    #define P25Q16H                                                                \
    {                                                                              \
        .total_size = 2L*1024L*1024L,                                              \
        .start_up_time_us = 12000, .manufacturer_id = 0x85,                        \
        .memory_type = 0x60, .capacity = 0x15, .max_clock_speed_mhz = 104,         \
        .quad_enable_bit_mask = 0x02, .has_sector_protection = false,              \
        .supports_fast_read = true, .supports_qspi = true,                         \
        .supports_qspi_writes = true, .write_status_register_split = false,        \
        .single_status_byte = false, .is_fram = false,                             \
    }

Add that to the bottom of `.....\libraries\Adafruit_SPIFlash\src\flash_devices.h`

In the file `.....\libraries\Adafruit_SPIFlash\src\Adafruit_SPIFlashBase.cpp` , look for `static const SPIFlash_Device_t possible_devices[] = {`

Add `P25Q16H` to the bottom of `possible_devices`
