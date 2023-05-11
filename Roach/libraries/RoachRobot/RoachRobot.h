#ifndef _ROACHROBOT_H_
#define _ROACHROBOT_H_

#include <Arduino.h>
#include <RoachLib.h>

extern nRF52RcRadio radio;
extern roach_telem_pkt_t telem_pkt;
extern roach_nvm_gui_desc_t** cfggroup_rxall;

#endif
