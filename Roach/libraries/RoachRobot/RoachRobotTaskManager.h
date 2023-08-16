#ifndef _ROACHROBOTTASKMANAGER_H_
#define _ROACHROBOTTASKMANAGER_H_

#include "RoachRobot.h"

// these extern functions must be implemented by the application code

extern void rtmgr_onPreFailed(void);           // called once, on the start of failsafe
extern void rtmgr_taskPreFailed(void);         // called repeatedly for 200ms after entering failsafe, at periodic interval
extern void rtmgr_onPostFailed(void);          // called once, after 200ms since failsafe
extern void rtmgr_taskPostFailed(void);        // called repeatedly after rtmgr_onPostFailed, at periodic interval
extern void rtmgr_onSafe(bool full_init);      // called once, upon returning from a failsafe state (and also upon radio connection)
extern void rtmgr_taskPeriodic(bool has_cmd);  // called repeatedly at the requested time interval, has_cmd means the radio has a command


// these functions are called by the application code

// intv is the interval that rtmgr_taskPeriodic should be called, set this to be about the time interval of each radio message
// timeout is the timeout since the last radio message, set to 0 to use radio.isConnected instead
void rtmgr_init(uint32_t intv, uint32_t timeout);

// call this task periodically from the application loop, supply current time in milliseconds, returns true if a task is actually called
bool rtmgr_task(uint32_t now);

// end the task manager permanently, goes through the entire failsafe sequence
void rtmgr_permEnd(void);

// returns 1 if ending, 2 if ended
uint8_t rtmgr_hasEnded(void);

// tricks the task manager to always run rtmgr_taskPeriodic even if the radio is not connected
void rtmgr_setSimulation(bool);

#endif
