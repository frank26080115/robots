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

extern roach_nvm_gui_desc_t cfgdesc_rf[];

int roachnvm_cntgroup(roach_nvm_gui_desc_t* g);
int roachnvm_rx_getcnt(void);
roach_nvm_gui_desc_t* roachnvm_rx_getAt(int idx);

int32_t roachnvm_getval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm);
void roachnvm_setval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t val);
void roachnvm_setval_inner(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t val, bool def_if_wrong);
int32_t roachnvm_incval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t x);
void roachnvm_valValidate(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm);
bool roachnvm_parseitem(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl, char* name, char* value);
bool roachnvm_parsecmd(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl, char* str);
void roachnvm_readfromfile(RoachFile* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_formatitem(char* str, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm);
void roachnvm_writetofile(RoachFile* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_writedescfile(RoachFile* f, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_setdefaults(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_debugNvm(Stream* stream, uint8_t* struct_ptr, uint32_t struct_sz, roach_nvm_gui_desc_t* desc_tbl);
void roachnvm_validateAll(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);

int32_t roach_reduce_to_scale(int32_t x);
int32_t roach_reduce_to_scale_2(int32_t x);
int32_t roach_reduce_to_scale_3(int32_t x);
int32_t roach_multiply_with_scale(int32_t a, int32_t b);
int32_t roach_lpf(int32_t nval, int32_t oval_x, int32_t flt);
int32_t roach_value_clamp(int32_t x, int32_t upper, int32_t lower);
static inline int32_t roach_value_clamp_abs(int32_t x, int32_t upper) { return roach_value_clamp(x, upper, -upper); }
int32_t roach_value_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max, bool limit);
int roach_div_rounded(const int n, const int d);
double roach_expo_curve(double x, double curve);
int32_t roach_expo_curve32(int32_t x, int32_t curve);
uint32_t roachnvm_getDescCnt(roach_nvm_gui_desc_t* desc_tbl);
uint32_t roach_crcCalc(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc);
uint32_t roachnvm_getConfCrc(uint8_t* data, roach_nvm_gui_desc_t* desc_tbl);
uint16_t roachnvm_getDescCrc(roach_nvm_gui_desc_t* desc_tbl);

#define ROACH_WRAP_ANGLE(x, m) do { while ((x) > (180 * (m))) { (x) -= 360 * (m); } while ((x) < -(180 * (m))) { (x) += 360 * (m); } } while (0)

#endif
