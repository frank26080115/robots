#ifndef _ROACH_DEFS_H_
#define _ROACH_DEFS_H_

#include <stdint.h>
#include <stdbool.h>

#include "roach_conf.h"

#define PACK_STRUCT __attribute__ ((packed))

#if defined(ESP32)
#define RoachFile File
#elif defined(NRF52840_XXAA)
#define RoachFile FatFile
#endif

#endif
