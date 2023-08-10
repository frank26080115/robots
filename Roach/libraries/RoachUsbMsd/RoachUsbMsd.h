#ifndef _ROACHUSBMSD_H_
#define _ROACHUSBMSD_H_

#include <Arduino.h>

void RoachUsbMsd_begin(void);                 // note: also starts the serial port
void RoachUsbMsd_task(void);                  // does house keeping, schedules connections and disconnections
void RoachUsbMsd_presentUsbMsd(void);         // call to start USB flash drive
void RoachUsbMsd_unpresent(void);             // call to stop  USB flash drive
bool RoachUsbMsd_isReady(void);               // checks if the file-system is formatted
bool RoachUsbMsd_isUsbPresented(void);        // checks if USB flash drive mode is active
bool RoachUsbMsd_hasChange(bool clr);         // checks if the PC has modified anything in the flash
bool RoachUsbMsd_hasVbus(void);               // checks if VBUS is available
bool RoachUsbMsd_canSave(void);               // checks if other firmware code is allowed to write things to file
uint32_t RoachUsbMsd_lastActivityTime(void);  // last time stamp of write activity from USB
uint64_t RoachUsbMsd_getFreeSpace(void);      // gets the free space inside the flash

#ifdef __cplusplus
extern "C" {
#endif

extern bool tud_connected(void);
extern bool tud_mounted(void);

#ifdef __cplusplus
}
#endif

#endif
