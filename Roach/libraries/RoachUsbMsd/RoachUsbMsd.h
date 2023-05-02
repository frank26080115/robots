#ifndef _ROACHUSBMSD_H_
#define _ROACHUSBMSD_H_

#include <Arduino.h>

void RoachUsbMsd_begin(void);
void RoachUsbMsd_task(void);
bool RoachUsbMsd_isReady(void);
bool RoachUsbMsd_hasChange(bool clr);

#endif
