#ifndef _ROACHUSBMSD_H_
#define _ROACHUSBMSD_H_

#include <Arduino.h>

void RoachUsbMsd_begin(void);
void RoachUsbMsd_task(void);
bool RoachUsbMsd_isReady(void);
bool RoachUsbMsd_hasChange(bool clr);
bool RoachUsbMsd_hasVbus(void);

#ifdef __cplusplus
extern "C" {
#endif

extern bool tud_connected(void);
extern bool tud_mounted(void);

#ifdef __cplusplus
}
#endif

#endif
