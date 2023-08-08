#ifndef _ROACHROBOTTASKMANAGER_H_
#define _ROACHROBOTTASKMANAGER_H_

#include "RoachRobot.h"

extern void rtmgr_onPreFailed(void);
extern void rtmgr_taskPreFailed(void);
extern void rtmgr_onPostFailed(void);
extern void rtmgr_taskPostFailed(void);
extern void rtmgr_onSafe(bool full_init);
extern void rtmgr_taskPeriodic(bool has_cmd);

void rtmgr_init(uint32_t intv, uint32_t timeout);
void rtmgr_task(uint32_t now);

#endif
