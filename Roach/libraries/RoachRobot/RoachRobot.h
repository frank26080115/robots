#ifndef _ROACHROBOT_H_
#define _ROACHROBOT_H_

#include <Arduino.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>

extern nRF52RcRadio radio;
extern roach_telem_pkt_t telem_pkt;
extern roach_nvm_gui_desc_t* cfggroup_rxall[];
extern roach_nvm_gui_desc_t cfggroup_rf[];
extern roach_nvm_gui_desc_t cfggroup_drive[];
extern roach_nvm_gui_desc_t cfggroup_weap[];
extern roach_nvm_gui_desc_t cfggroup_sensor[];

extern uint32_t nvm_checksum;
extern uint8_t* nvm_ptr8;
extern uint32_t roachrobot_nvmSize;

void roachrobot_handleFileLoad(void* cmd, char* argstr, Stream* stream);
void roachrobot_handleFileSave(void* cmd, char* argstr, Stream* stream);
bool roachrobot_saveSettingsToFile(const char* fname, uint8_t* data);
bool roachrobot_loadSettingsFile(const char* fname, uint8_t* data);
bool roachrobot_loadSettings(uint8_t* data);
bool roachrobot_saveSettings(uint8_t* data);
void roachrobot_defaultSettings(uint8_t* data);

void roachrobot_recalcChecksum(void);

#endif
