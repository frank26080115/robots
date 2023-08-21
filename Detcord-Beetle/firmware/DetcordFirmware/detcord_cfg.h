#ifndef _DETCORD_CFG_H_
#define _DETCORD_CFG_H_

#define USE_DSHOT

#define ROACHPKTFLAG_WEAPON ROACHPKTFLAG_BTN1
#define ROACHPKTFLAG_IMU    ROACHPKTFLAG_BTN3

//#define debug_printf(...)            // do nothing
#define debug_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__); } while (0)

#define DEVMODE_PERIODIC_DEBUG 250
//#define DEVMODE_DEBUG_RX_ERRORS
//#define DEVMODE_WAIT_SERIAL_RUN
//#define DEVMODE_WAIT_SERIAL_INIT
#define DEVMODE_AVOID_USBMSD
#define DEVMODE_DISABLE_IMU

#endif
