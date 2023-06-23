#include "detcord_inc.h"
#include <Adafruit_TinyUSB.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachCmdLine.h>
#include <RoachUsbMsd.h>
#include <RoachPerfCnt.h>
#include <RoachServo.h>
#include <RoachIMU.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <AsyncAdc.h>
#include <nRF5Rand.h>
#include <NonBlockingTwi.h>

bool can_start = false;

FatFile fatroot;
FatFile fatfile;
extern RoachCmdLine cmdline;

nRF52RcRadio radio = nRF52RcRadio(false);
roach_ctrl_pkt_t  rx_pkt    = {0};
roach_telem_pkt_t telem_pkt = {0};

RoachServo drive_left;
RoachServo drive_right;
RoachServo weapon;

extern roach_nvm_gui_desc_t nvm_desc[];
roach_rf_nvm_t nvm_rf;
detcord_nvm_t nvm_robot;

void setup()
{
    
}

void loop()
{
    
}
