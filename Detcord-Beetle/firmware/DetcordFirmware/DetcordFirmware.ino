#include "detcord_inc.h"
#include <Adafruit_TinyUSB.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachRobot.h>
#include <RoachRobotTaskManager.h>
#include <RoachCmdLine.h>
#include <RoachUsbMsd.h>
#include <RoachPerfCnt.h>
#include <RoachPot.h>
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

RoachServo drive_left (DETCORDHW_PIN_SERVO_DRV_L);
RoachServo drive_right(DETCORDHW_PIN_SERVO_DRV_R);
#ifdef USE_DSHOT
    nRF52Dshot
#else
    RoachServo
#endif
    weapon(DETCORDHW_PIN_SERVO_WEAP);

#ifdef ROACHIMU_USE_BNO085
RoachIMU_BNO imu(5000, 0, ROACHIMU_ORIENTATION_XYZ, BNO08x_I2CADDR_DEFAULT, DETCORDHW_PIN_IMU_RST);
#endif
#ifdef ROACHIMU_USE_LSM6DS3
RoachIMU_LSM imu(ROACHIMU_ORIENTATION_XYZ);
#endif

RoachPID pid;
RoachDriveMixer mixer;

RoachPot battery(DETCORDHW_PIN_ADC_BATT, NULL);

extern roach_nvm_gui_desc_t detcord_cfg_desc[];
roach_rf_nvm_t nvm_rf;
detcord_nvm_t nvm;

RoachHeadingManager heading_mgr((uint32_t*)&(nvm.heading_timeout));

#ifdef BOARD_IS_ITSYBITSY
RoachHeartbeat hb_red = RoachHeartbeat(DETCORDHW_PIN_LED);
RoachDotStar   hb_rgb = RoachDotStar();
#endif

#ifdef BOARD_IS_XIAOBLE
void hb_cb(bool);
RoachHeartbeat hb_red = RoachHeartbeat(hb_cb);
RoachRgbLed    hb_rgb = RoachRgbLed();
#endif

void setup()
{
    hb_red.begin();
    hb_rgb.begin();

    settings_init();
    RoachUsbMsd_begin(); // this also starts the serial port
    if (RoachUsbMsd_hasVbus())
    {
        RoachUsbMsd_presentUsbMsd();
    }

    battery.begin();

    nbtwi_init(DETCORDHW_PIN_I2C_SCL, DETCORDHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE);
    imu.begin();

    pid.cfg = &(nvm.pid_heading);
    mixer.cfg_left = &(nvm.drive_left);
    mixer.cfg_right = &(nvm.drive_right);

    radio.begin();
    radio.config(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt);

    PerfCnt_init();
    RoachWdt_init(500);

    rtmgr_init(10, 1000);

    roachrobot_init((uint8_t*)&nvm, (uint32_t)sizeof(nvm), (roach_nvm_gui_desc_t*)detcord_cfg_desc, (uint32_t)sizeof(detcord_cfg_desc));

    debug_printf("Detcord finished setup()\r\n");
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
    RoachPot_allTask();
    settings_task();
    #ifdef PERFCNT_ENABLED
    PerfCnt_task();
    #endif
}

void rtmgr_taskPeriodic(bool has_cmd) // this either happens once per radio message (10 ms) or 12 ms
{
    if (has_cmd) {
        radio.read((uint8_t*)&rx_pkt);
    }

    int32_t gyro_corr = 0;
    if ((rx_pkt.flags & ROACHPKTFLAG_GYROACTIVE) != 0)
    {
        if (imu.isReady() == false || imu.hasFailed()) {
            heading_mgr.setReset();
            pid.reset();
        }
        #ifndef ROACHIMU_AUTO_MATH
        else if (imu.hasNew(false)) {
            imu.doMath();
        }
        #endif
        heading_mgr.task(&rx_pkt, imu.heading, radio.getSessionId());

        gyro_corr = pid.compute(heading_mgr.getCurHeading(), heading_mgr.getTgtHeading());
    }
    else
    {
        gyro_corr = 0;
        heading_mgr.setReset();
        pid.reset();
    }

    mixer.mix(rx_pkt.throttle, rx_pkt.steering, roach_reduce_to_scale(gyro_corr));
    drive_left.writeMicroseconds(mixer.getLeft());
    drive_right.writeMicroseconds(mixer.getRight());

    if ((rx_pkt.flags & ROACHPKTFLAG_WEAPON) != 0)
    {
        #ifdef USE_DSHOT
        weapon.setThrottle(roach_value_clamp(roach_multiply_with_scale(rx_pkt.pot_weap, nvm.weapon.scale), nvm.weapon.limit_max, 0));
        #else
        weapon.writeMicroseconds(roach_value_clamp(nvm.weapon.limit_min + (roach_multiply_with_scale(rx_pkt.pot_weap, nvm.weapon.scale)), nvm.weapon.limit_max, nvm.weapon.limit_min));
        #endif
    }
    else
    {
        #ifdef USE_DSHOT
        weapon.setThrottle(0);
        #else
        weapon.writeMicroseconds(nvm.weapon.limit_min);
        #endif
    }
    #ifdef USE_DSHOT
    weapon.task();
    #endif

    if (has_cmd)
    {
        RoSync_task(); // there's no point in calling this from robot_tasks, the radio telemetry transmissions only happen on successful reception
        roachrobot_pipeCmdLine();

        telem_pkt.battery = roach_value_map(battery.getAdcFiltered(), 0, DETCORDHW_BATT_ADC_4200MV, 0, 4200, false);
        telem_pkt.heading = (imu.isReady() == false) ? ROACH_HEADING_INVALID_NOTREADY : ((imu.hasFailed()) ? ROACH_HEADING_INVALID_HASFAILED : ((uint16_t)lround(imu.heading * ROACH_ANGLE_MULTIPLIER)));
        roachrobot_telemTask(); // this will fill out common fields in the telem packet and then send it off
    }
}

void rtmgr_onPreFailed(void)
{
    heading_mgr.setReset();
    pid.reset();
    mixer.mix(0, 0, 0);
    drive_left.writeMicroseconds(mixer.getLeft());
    drive_right.writeMicroseconds(mixer.getRight());
    #ifdef USE_DSHOT
    weapon.setThrottle(0);
    #else
    weapon.writeMicroseconds(nvm.weapon.limit_min);
    #endif
}

void rtmgr_taskPreFailed(void)
{
    #ifdef USE_DSHOT
    weapon.task();
    #endif
    RoSync_task();
}

void rtmgr_onPostFailed(void)
{
    drive_left.detach();
    drive_right.detach();
    weapon.detach();
}

void rtmgr_taskPostFailed(void)
{
    RoSync_task();
}

void rtmgr_onSafe(bool full_init)
{
    if (full_init)
    {
        drive_left.begin();
        drive_right.begin();
        weapon.begin();
    }
    mixer.mix(0, 0, 0);
    drive_left.writeMicroseconds(mixer.getLeft());
    drive_right.writeMicroseconds(mixer.getRight());
    #ifdef USE_DSHOT
    weapon.setThrottle(0);
    #else
    weapon.writeMicroseconds(nvm.weapon.limit_min);
    #endif
    heading_mgr.setReset();
    pid.reset();
}
