#ifndef _ROACHPERFCNT_H_
#define _ROACHPERFCNT_H_

#include <Arduino.h>
#include <stdint.h>

#define PERFCNT_ENABLED

void PerfCnt_init(void);
#ifdef PERFCNT_ENABLED
void PerfCnt_task(void);
int  PerfCnt_get(void);
int  PerfCnt_ram(void);
#else
#define PerfCnt_task()
#define PerfCnt_get() (0)
#define PerfCnt_ram() (0)
#endif

#endif
