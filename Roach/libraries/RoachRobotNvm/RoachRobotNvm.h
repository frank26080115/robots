#ifndef _ROACHROBOTNVM_H_
#define _ROACHROBOTNVM_H_

#include <Arduino.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>

extern uint32_t robotnvm_chksum;
extern uint32_t robotnvm_desc_chksum;
extern bool robotnvm_hasUpdate;

bool robotnvm_init(roach_nvm_gui_desc_t* desc, void* nvm_ptr, uint32_t nvm_sz);
bool robotnvm_task(nRF52RcRadio* radio);
bool robotnvm_genDescFile(const char* fname, roach_nvm_gui_desc_t* desc_tbl, bool force);
void robotnvm_setDefaults(void);
bool robotnvm_saveToFile(const char* fname);
bool robotnvm_loadFromFile(const char* fname);
void robotnvm_sendDescFileStart(void);
void robotnvm_sendDescFileChunk(nRF52RcRadio* radio);
void robotnvm_sendConfStart(void);
void robotnvm_sendConfChunk(nRF52RcRadio* radio);
bool robotnvm_handlePkt(radio_binpkt_t* pkt);

#endif
