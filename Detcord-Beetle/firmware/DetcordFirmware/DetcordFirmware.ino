#include "detcord_inc.h"
#include <Adafruit_TinyUSB.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachCmdLine.h>
#include <RoachUsbMsd.h>
#include <RoachPerfCnt.h>
#include <RoachServo.h>
#include <RoachIMU.h>
#include <RoachHeadingManager.h>
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

RoachServo drive_left(DETCORDHW_PIN_SERVO_DRV_L);
RoachServo drive_right(DETCORDHW_PIN_SERVO_DRV_R);
RoachServo weapon(DETCORDHW_PIN_SERVO_WEAP);

RoachIMU imu(5000, 0, 0, BNO08x_I2CADDR_DEFAULT, -1);

extern roach_nvm_gui_desc_t nvm_desc[];
roach_rf_nvm_t nvm_rf;
detcord_nvm_t nvm_robot;

RoachHeadingManager heading_mgr((uint32_t*)&(nvm_robot.heading_timeout));

void setup()
{
    RoachUsbMsd_begin();
    if (RoachUsbMsd_hasVbus())
    {
        RoachUsbMsd_presentUsbMsd();
    }

    drive_left.begin();
    drive_right.begin();
    weapon.begin();

    nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE);
    imu.begin();

    radio.begin();
    radio.config(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt);
}

void loop()
{
    
}

void robot_task()
{
    radio.task();
    nbtwi_task();
    imu.task();
    RoachUsbMsd_task();
}