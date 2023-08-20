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
#include <Wire.h>

FatFile fatroot;
FatFile fatfile;
extern RoachCmdLine cmdline;

RoachServo drive_left (DETCORDHW_PIN_SERVO_DRV_L);
RoachServo drive_right(DETCORDHW_PIN_SERVO_DRV_R);
#ifdef USE_DSHOT
    nRF52Dshot weapon(DETCORDHW_PIN_SERVO_WEAP, 9);
    // interval is set to 9ms, but the weapon task is placed in a low priority task that's called every 10ms, so it will force a send every single time
#else
    RoachServo weapon(DETCORDHW_PIN_SERVO_WEAP);
#endif

bool servos_has_inited = false;
bool force_servo_outputs = false;

#ifdef ROACHIMU_USE_BNO085
RoachIMU_BNO imu(5000, 0, ROACHIMU_ORIENTATION_XYZ, BNO08x_I2CADDR_DEFAULT, DETCORDHW_PIN_IMU_RST);
#endif
#ifdef ROACHIMU_USE_LSM6DS3
//RoachIMU_LSM imu(ROACHIMU_ORIENTATION_XYZ);
RoachIMU_LSM imu(ROACHIMU_ORIENTATION_XYZ, LSM6DS3_I2CADDR_DEFAULT, ROACHIMU_DEF_PIN_PWR, -1);
#endif

RoachPID pid;
RoachDriveMixer mixer;

RoachPot battery(DETCORDHW_PIN_ADC_BATT, NULL);

extern roach_nvm_gui_desc_t detcord_cfg_desc[];
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

#ifdef DEVMODE_PERIODIC_DEBUG
detcord_runtime_log_t dbglog;
bool log_active = true;
#endif

void setup()
{
    //sd_softdevice_disable();

    #ifdef DEVMODE_WAIT_SERIAL_INIT
    Serial.begin(19200);
    while (Serial.available() == 0) {
        Serial.printf("Detcord FW waiting for input\r\n");
        delay(100);
    }
    #endif

    hb_red.begin();
    hb_rgb.begin();

    bool flash_ok = RoachUsbMsd_begin(); // this also starts the serial port
    settings_init();
    #if !defined(DEVMODE_AVOID_USBMSD) && !defined(BOARD_IS_XIAOBLE)
    // XIAO has VBUS connected to the external power
    // so it is literally impossible to tell if the VBUS is from a computer or from a battery
    if (RoachUsbMsd_hasVbus())
    {
        RoachUsbMsd_presentUsbMsd();
    }
    #endif

    battery.alt_filter = ROACH_FILTER_DEFAULT;
    battery.begin();

    #ifdef BOARD_IS_XIAOBLE
    XiaoBleSenseLsm_powerOn(ROACHIMU_DEF_PIN_PWR);
    #endif

    nbtwi_init(DETCORDHW_PIN_I2C_SCL, DETCORDHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE, DETCORDHW_I2C_FAST);
    imu.begin();

    pid.cfg = &(nvm.pid_heading);
    mixer.cfg_left = &(nvm.drive_left);
    mixer.cfg_right = &(nvm.drive_right);

    PerfCnt_init();
    RoachWdt_init(500);

    roachrobot_init((uint8_t*)&nvm, (uint32_t)sizeof(nvm), (roach_nvm_gui_desc_t*)detcord_cfg_desc, settings_getDescSize());

    rtmgr_init(10, 1000);

    #ifdef DEVMODE_WAIT_SERIAL_RUN
    while (cmdline.has_interaction() == false) {
        Serial.printf("Detcord FW waiting for input\r\n");
        waitFor(100);
    }
    #endif

    #if defined(DEVMODE_WAIT_SERIAL_RUN) || defined(DEVMODE_WAIT_SERIAL_INIT)
    Serial.printf("Detcord Setup Finished\r\n");
    #endif
}

void loop()
{
    uint32_t now = millis();
    RoachWdt_feed();
    robot_tasks(now); // short tasks
    if (rtmgr_task(now)) // this will: call 10ms periodic task, and handle failsafe events
    {
        // if return true, then run low priority tasks
        robot_lowPriorityTasks();

        #ifdef DEVMODE_PERIODIC_DEBUG
        if (log_active) {
            //logger_gather();
            logger_report(now);
        }
        #endif
    }
}

void robot_tasks(uint32_t now) // short but high priority tasks
{
    radio.task();
    nbtwi_task();
    imu.task();
    cmdline.task();
    RoachPot_allTask();
    #ifdef PERFCNT_ENABLED
    PerfCnt_task();
    #endif
}

void robot_lowPriorityTasks(void)
{
    heartbeat_task();
    settings_task();
    RoachUsbMsd_task();
    RoSync_task();
    #ifdef USE_DSHOT
    weapon.task();
    #endif
}

void rtmgr_taskPeriodic(bool has_cmd) // this either happens once per radio message (10 ms) or 12 ms
{
    if (has_cmd) {
        radio.read((uint8_t*)&rx_pkt);
    }

    #ifdef DEVMODE_PERIODIC_DEBUG
    dbglog.has_cmd = has_cmd ? 1 : 0;
    #endif

    int32_t gyro_corr = 0;
    if ((rx_pkt.flags & ROACHPKTFLAG_IMU) != 0)
    {
        if (imu.isReady() == false || imu.hasFailed() || imu.getErrorOccured(true)) {
            heading_mgr.setReset();
            pid.reset();
        }
        else
        {
            mixer.setUpsideDown(imu.is_inverted);
            #ifndef ROACHIMU_AUTO_MATH
            if (imu.hasNew(false)) {
                imu.doMath();
            }
            #endif
        }
        heading_mgr.task(&rx_pkt, imu.heading, radio.getSessionId());

        gyro_corr = pid.compute(heading_mgr.getCurHeading(), heading_mgr.getTgtHeading());
    }
    else
    {
        mixer.setUpsideDown(false);
        gyro_corr = 0;
        heading_mgr.setReset();
        pid.reset();
    }

    if ((rx_pkt.flags & ROACHPKTFLAG_SAFE) == 0)
    {
        mixer.mix(rx_pkt.throttle, rx_pkt.steering, gyro_corr);
    }
    else
    {
        heading_mgr.setReset();
        pid.reset();
        mixer.mix(0, 0, 0);
    }
    drive_left .writeMicroseconds(mixer.getLeft ());
    drive_right.writeMicroseconds(mixer.getRight());

    if ((rx_pkt.flags & ROACHPKTFLAG_WEAPON) != 0 && (rx_pkt.flags & ROACHPKTFLAG_SAFE) == 0)
    {
        weapon.
        #ifdef USE_DSHOT
            setThrottle(
        #else
            writeMicroseconds(
        #endif
                RoachServo_calc(rx_pkt.pot_weap, &(nvm.weapon)));
    }
    else
    {
        #ifdef USE_DSHOT
        weapon.setThrottle(0);
        #else
        weapon.writeMicroseconds(nvm.weapon.limit_min);
        #endif
    }

    if (has_cmd)
    {
        RoSync_task(); // there's no point in calling this too often, the radio telemetry transmissions only happen on successful reception
        roachrobot_pipeCmdLine();

        telem_pkt.battery = roach_value_map(battery.getAdcFiltered(), 0, DETCORDHW_BATT_ADC_4200MV, 0, 4200, false);
        telem_pkt.heading = (imu.isReady() == false) ? ROACH_HEADING_INVALID_NOTREADY : ((imu.hasFailed()) ? ROACH_HEADING_INVALID_HASFAILED : ((uint16_t)lround(imu.heading * ROACH_ANGLE_MULTIPLIER)));
        roachrobot_telemTask(millis()); // this will fill out common fields in the telem packet and then send it off
    }
}

void rtmgr_taskPreStart(void)
{
    #ifndef ROACHIMU_AUTO_MATH
    if (imu.hasNew(false)) {
        imu.doMath();
    }
    #endif
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
    #ifdef DEVMODE_PERIODIC_DEBUG
    dbglog.has_cmd = -1;
    #endif
}

void rtmgr_onPostFailed(void)
{
    if (force_servo_outputs == false) // prevent failsafe from kicking on if we are doing signal testing
    {
        drive_left .detach();
        drive_right.detach();
        weapon     .detach();
        servos_has_inited = false;
    }
}

void rtmgr_taskPostFailed(void)
{
    #ifdef DEVMODE_PERIODIC_DEBUG
    dbglog.has_cmd = -1;
    #endif
}

void rtmgr_onSafe(bool full_init)
{
    // ignore full_init
    if (servos_has_inited == false)
    {
        drive_left.begin();
        drive_right.begin();
        weapon.begin();
        servos_has_inited = true;
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

void calibgyro_func(void* cmd, char* argstr, Stream* stream)
{
    #ifdef BOARD_IS_XIAOBLE
    if (atoi(argstr) == 123) {
        if (imu.calib != NULL) {
            Serial.printf("[%u] IMU calib = [ %d , %d , %d ]\r\n", millis(), imu.calib->x, imu.calib->y, imu.calib->z);
        }
        else {
            Serial.printf("[%u] IMU has no calibration data yet\r\n", millis());
        }
        return;
    }
    #endif
    imu.tare();
    heading_mgr.setReset();
    pid.reset();
    Serial.printf("[%u] IMU tare start\r\n", millis());
}

void robot_force_servos_on(void)
{
    force_servo_outputs = true;
    rtmgr_onSafe(true);
}

void logger_gather(void)
{
    #ifdef DEVMODE_PERIODIC_DEBUG
    dbglog.cur_heading = heading_mgr.getCurHeading();
    dbglog.tgt_heading = heading_mgr.getTgtHeading();
    dbglog.gyro_corr   = pid.getLastOutput();
    dbglog.drv_left    = drive_left .readMicroseconds();
    dbglog.drv_right   = drive_right.readMicroseconds();

    dbglog.weapon = weapon.
    #ifdef USE_DSHOT
        readThrottle
    #else
        readMicroseconds
    #endif
        ();

    dbglog.throttle    = rx_pkt.throttle;
    dbglog.steering    = rx_pkt.steering;
    dbglog.battery     = telem_pkt.battery;
    dbglog.rssi        = telem_pkt.rssi;
    dbglog.loss_rate   = telem_pkt.loss_rate;
    dbglog.imu_heading = telem_pkt.heading;
    #ifndef DEVMODE_NO_RADIO
    dbglog.session_id  = radio.getSessionId();
    #endif
    #endif
}

void logger_report(uint32_t now)
{
    #ifdef DEVMODE_PERIODIC_DEBUG
    static uint32_t last_debug_time = 0;

    if ((now - last_debug_time) >= DEVMODE_PERIODIC_DEBUG)
    {
        last_debug_time = now;
        logger_gather();
        Serial.printf("LOG[%u]:, ", now);
        int16_t* log_ptr = (int16_t*)&dbglog;
        int log_cnt = sizeof(dbglog)/sizeof(int16_t);
        int log_i;
        for (log_i = 0; log_i < log_cnt; log_i++) {
            Serial.printf("%d , ", log_ptr[log_i]);
        }
        Serial.printf("\r\n");
    }
    #endif
}