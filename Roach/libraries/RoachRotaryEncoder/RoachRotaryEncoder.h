#ifndef _ROACHROTARYENCODER_H_
#define _ROACHROTARYENCODER_H_

#include <Arduino.h>

#define ROACHENC_USE_QDEC
//#define ROACHENC_USE_GPIO
//#define ROACHENC_MODE_COMPLEX

#if defined(ROACHENC_USE_GPIO)
void IRAM_ATTR RoachEnc_task(void);
#elif defined(ROACHENC_USE_QDEC)
void RoachEnc_task(void);
#else
#define RoachEnc_task()
#endif

void RoachEnc_begin(int pin_a, int pin_b);
int32_t RoachEnc_get(bool clr); // outputs raw encoder tick numbers
bool RoachEnc_hasMoved(bool clr);
uint32_t RoachEnc_getLastTime(void);
void RoachEnc_simulate(int x);

#endif
