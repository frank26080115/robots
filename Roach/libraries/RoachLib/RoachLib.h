#ifndef _ROACHLIB_H_
#define _ROACHLIB_H_

#include <Arduino.h>

#include "roach_conf.h"
#include "roach_defs.h"
#include "roach_types.h"

#if defined(ESP32)
#include <SPIFFS.h>
#define RoachFile File
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#define RoachFile FatFile
#endif

extern roach_nvm_gui_desc_t cfggroup_rf[];
extern roach_nvm_gui_desc_t cfggroup_drive[];
extern roach_nvm_gui_desc_t cfggroup_weap[];
extern roach_nvm_gui_desc_t cfggroup_imu[];

int roachnvm_cntgroup(roach_nvm_gui_desc_t* g);
void roachnvm_buildrxcfggroup(void);
int roachnvm_rx_getcnt(void);
roach_nvm_gui_desc_t* roachnvm_rx_getAt(int idx);

int32_t roachnvm_getval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm);
void roachnvm_setval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t val);
int32_t roachnvm_incval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t x);
bool roachnvm_parseitem(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl, char* name, char* value);
void roachnvm_readfromfile(RoachFile* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_formatitem(char* str, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm);
void roachnvm_writetofile(RoachFile* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_writedescfile(RoachFile* f, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_setdefaults(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);

int32_t roach_reduce_to_scale(int32_t x);
int32_t roach_multiply_with_scale(int32_t a, int32_t b);
int32_t roach_lpf(int32_t nval, int32_t oval_x, int32_t flt);
int32_t roach_value_clamp(int32_t x, int32_t upper, int32_t lower);
int32_t roach_value_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max, bool limit);
int roach_div_rounded(const int n, const int d);
double roach_expo_curve(double x, double curve);
int32_t roach_expo_curve32(int32_t x, int32_t curve);
uint32_t roach_crcCalc(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc);

void roach_drive_mix(int32_t throttle, int32_t steering, int32_t gyro_correction, uint8_t flip, int32_t* output_left, int32_t* output_right, roach_nvm_servo_t* cfg);
int32_t roach_drive_applyServoParams(int32_t spd, roach_nvm_servo_t* cfg);
int32_t roach_ctrl_cross_mix(int32_t throttle, int32_t steering, uint32_t mix);

#endif
