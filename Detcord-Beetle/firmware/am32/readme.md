Detcord uses a weapon ESC that runs [AM32 firmware](https://github.com/AlkaMotors/AM32-MultiRotor-ESC-firmware). The specific ESC is a [Aria 70A model from ItGresa](https://itgresa.com/product/aria-70a-blheli-brushless-speed-controller/), it is also sold under other brands.

This directory contains the firmware and configuration for this ESC.

The bootloader has these customizations:

 * UART uses two stop bits, for more reliable transmission
 * extended serial port timeout from 250 us to 1000 us, for more reliable transmission
 * note: these changes make the total transfer slightly slower
 * LED is a teal colour when bootloader is active

The firmware has these customizations:

 * optionally, the ESC can be always armed, for more immediate startup if the ESC reboots in the middle of a fight
 * RGB LED blinking animations implemented
 * current sensor calibration
 * configurable duty cycle ramp-up and ramp-down rates, for more reliable heavy mass start-up
 * sinusoidal start-up mode is dynamically disengaged if throttle is going down

The EEPROM has these customizations:

 * sine startup, for low speed startup spin of heavy mass
 * additional settings for duty cycle ramping
 * stall protection is off, seems to make the ESC not work at low speeds
 * low voltage cutoff is off, this ESC does not have the ADC input connected to the battery at all
 * current limit set to 68A
