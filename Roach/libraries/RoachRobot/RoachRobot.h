#ifndef _ROACHROBOT_H_
#define _ROACHROBOT_H_

#include <Arduino.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>

extern nRF52RcRadio radio;
extern roach_telem_pkt_t telem_pkt;
extern roach_nvm_gui_desc_t* cfg_desc;
#define rosync_desc_tbl cfg_desc

extern uint32_t rosync_checksum_nvm;
extern uint32_t rosync_checksum_desc;
extern uint8_t* rosync_nvm;
extern uint32_t rosync_nvm_sz;
extern uint32_t cfg_desc_sz;

void roachrobot_init(uint8_t* nvm_ptr, uint32_t nvm_size, roach_nvm_gui_desc_t* desc_ptr, uint32_t desc_size);

void roachrobot_handleFileLoad(void* cmd, char* argstr, Stream* stream);
void roachrobot_handleFileSave(void* cmd, char* argstr, Stream* stream);
bool roachrobot_saveSettingsToFile(const char* fname, uint8_t* data);
bool roachrobot_loadSettingsFile(const char* fname, uint8_t* data);
bool roachrobot_loadSettings(uint8_t* data);
bool roachrobot_saveSettings(uint8_t* data);
void roachrobot_defaultSettings(uint8_t* data);

uint32_t roachrobot_recalcChecksum(void);
uint32_t roachrobot_calcDescChecksum(void);
void roachrobot_pipeCmdLine(void);
void roachrobot_telemTask(void);

#endif
