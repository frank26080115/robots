#ifndef _ROACHROTARYENCODER_H_
#define _ROACHROTARYENCODER_H_

#include <Arduino.h>

void RoachEnc_begin(int pin_a, int pin_b);
void IRAM_ATTR RoachEnc_isr(void);
void IRAM_ATTR RoachEnc_task(void);
int32_t RoachEnc_get(bool clr);
bool RoachEnc_hasMoved(bool clr);

#endif
