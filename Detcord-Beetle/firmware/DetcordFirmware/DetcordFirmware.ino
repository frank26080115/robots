#include "detcord_inc.h"
#include <Adafruit_TinyUSB.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachRobot.h>
#include <RoachRobotTaskManager.h>
#include <RoachCmdLine.h>
#include <RoachUsbMsd.h>
#include <RoachPerfCnt.h>
#include <RoachServo.h>
#include <nRF52Dshot.h>
#include <RoachIMU.h>
#include <RoachPID.h>
#include <RoachDriveMixer.h>
#include <RoachHeartbeat.h>
#include <RoachHeadingManager.h>
#include <nRF52OneWireSerial.h>
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
#ifdef USE_DSHOT
    nRF52Dshot
#else
    RoachServo
#endif
    weapon(DETCORDHW_PIN_SERVO_WEAP);

RoachIMU imu(5000, 0, 0, BNO08x_I2CADDR_DEFAULT, -1);
RoachPID pid;
RoachDriveMixer mixer;

extern roach_nvm_gui_desc_t nvm_desc[];
roach_rf_nvm_t nvm_rf;
detcord_nvm_t nvm;

RoachHeadingManager heading_mgr((uint32_t*)&(nvm.heading_timeout));

RoachHeartbeat hb_red = RoachHeartbeat(DETCORDHW_PIN_LED);
RoachRgbLed    hb_rgb = RoachRgbLed();

extern roach_nvm_gui_desc_t cfggroup_rf[];
extern roach_nvm_gui_desc_t cfggroup_drive[];
extern roach_nvm_gui_desc_t cfggroup_weap[];
extern roach_nvm_gui_desc_t cfggroup_sensor[];
roach_nvm_gui_desc_t* cfggroup_rxall[] = {
    (roach_nvm_gui_desc_t*)cfggroup_rf,
    (roach_nvm_gui_desc_t*)cfggroup_drive,
    (roach_nvm_gui_desc_t*)cfggroup_weap,
    (roach_nvm_gui_desc_t*)cfggroup_sensor,
    NULL,
};

void setup()
{
    hb_red.begin();
    hb_rgb.begin();

    settings_init();
    RoachUsbMsd_begin();
    if (RoachUsbMsd_hasVbus())
    {
        RoachUsbMsd_presentUsbMsd();
    }

    nbtwi_init(DETCORDHW_PIN_I2C_SCL, DETCORDHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE);
    imu.begin();

    pid.cfg = &(nvm.pid_heading);
    mixer.cfg_left = &(nvm.drive_left);
    mixer.cfg_right = &(nvm.drive_right);

    radio.begin();
    radio.config(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt);

    RoachWdt_init(500);

    rtmgr_init(10, 1000);
}

void loop()
{
    uint32_t now = millis();
    RoachWdt_feed();
    robot_tasks(now); // short tasks
    rtmgr_task(now); // this will: call 10ms periodic task, and handle failsafe events
}

void robot_tasks(uint32_t now) // short tasks
{
    radio.task();
    nbtwi_task();
    imu.task();
    RoachUsbMsd_task();
    cmdline.task();
    heartbeat_task();
    RoSync_task();
}

void rtmgr_taskPeriodic(bool has_cmd)
{
    if (has_cmd)
    {
        // TODO
    }
    mixer.mix(0, 0, 0); // TODO
    drive_left.writeMicroseconds(mixer.getLeft());
    drive_right.writeMicroseconds(mixer.getRight());
}

void rtmgr_onPreFailed(void)
{
    mixer.mix(0, 0, 0);
    drive_left.writeMicroseconds(mixer.getLeft());
    drive_right.writeMicroseconds(mixer.getRight());
    weapon.setThrottle(0);
}

void rtmgr_taskPreFailed(void)
{
    // do nothing
}

void rtmgr_onPostFailed(void)
{
    drive_left.detach();
    drive_right.detach();
    weapon.detach();
}

void rtmgr_taskPostFailed(void)
{
    // do nothing
}

void rtmgr_onSafe(bool full_init)
{
    if (full_init)
    {
        drive_left.begin();
        drive_right.begin();
        weapon.begin();
    }
    else
    {
        mixer.mix(0, 0, 0);
        drive_left.writeMicroseconds(mixer.getLeft());
        drive_right.writeMicroseconds(mixer.getRight());
        weapon.setThrottle(0);
    }
}
