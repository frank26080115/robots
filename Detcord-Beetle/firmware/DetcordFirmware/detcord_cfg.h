#ifndef _DETCORD_CFG_H_
#define _DETCORD_CFG_H_

#define USE_DSHOT

#define ROACHPKTFLAG_WEAPON ROACHPKTFLAG_BTN1

//#define debug_printf(...)            // do nothing
#define debug_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__) } while (0)

#endif
