#ifndef _ROACHROBOTPRIVATE_H_
#define _ROACHROBOTPRIVATE_H_

#include <Arduino.h>
#include <RoachLib.h>

//#define debug_printf(...)            // do nothing
#define debug_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__); } while (0)

#endif
