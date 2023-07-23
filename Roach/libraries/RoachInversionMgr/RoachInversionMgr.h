#ifndef _ROACHINVERSIONMGR_H_
#define _ROACHINVERSIONMGR_H_

#include <Arduino.h>
#include <RoachIMU.h>

#define RINVMGR_HYSTER_DEFAULT 15    // angle in degrees beyond 90 degrees that needs to be exceeded to cause a flip to be detected

bool rInvMgr_isInverted(euler* x);
bool rInvMgr_hasChangedInversion(bool clr);
void rInvMgr_setHyster(float x);

// RoachDriveMixer implements the swapping and reversing of the motor speed values
// TODO: check if yaw angle needs to be reversed for PID controller

#endif
