#ifndef _ROACHUSBMSD_H_
#define _ROACHUSBMSD_H_

#include <Arduino.h>

void RoachUsbMsd_begin(void);
void RoachUsbMsd_task(void);
void RoachUsbMsd_presentUsbMsd(void);
void RoachUsbMsd_unpresent(void);
bool RoachUsbMsd_isReady(void);
bool RoachUsbMsd_isUsbPresented(void);
bool RoachUsbMsd_hasChange(bool clr);
bool RoachUsbMsd_hasVbus(void);
bool RoachUsbMsd_canSave(void);
uint64_t RoachUsbMsd_getFreeSpace(void);

#ifdef __cplusplus
extern "C" {
#endif

extern bool tud_connected(void);
extern bool tud_mounted(void);

#ifdef __cplusplus
}
#endif

#endif
