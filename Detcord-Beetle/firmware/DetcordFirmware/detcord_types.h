#ifndef _DETCORD_TYPES_H_
#define _DETCORD_TYPES_H_

#include <RoachLib.h>

#include "detcord_cfg.h"
#include "detcord_defs.h"

typedef struct
{
    uint32_t magic;

    //uint8_t  imu_orientation; // there are 6 x 8 combinations, see RoachImu's implementation
    // note: IMU orientation is fixed by hardware now, not physically possible to be different

    roach_nvm_servo_t drive_left;
    bool              drive_left_flip;
    roach_nvm_servo_t drive_right;
    bool              drive_right_flip;
    roach_nvm_servo_t weapon;
    //roach_nvm_servo_t weapon2;

    roach_nvm_pid_t pid_heading;
    //roach_nvm_pid_t pid_spindown; // experiment: adjust weapon speed to turn the robot, like a satellite's reaction wheels
    //uint16_t spindown_limit;
    //uint32_t weap_rampup; // just in case the ESC is bad, slowly accelerate weapon, 0 to disable

    uint16_t heading_timeout; // when steering override is used, how long to timeout the heading-hold disabled mode

    uint32_t checksum;
}
PACK_STRUCT
detcord_nvm_t;

typedef struct
{
    int16_t has_cmd;
    int16_t throttle;
    int16_t steering;
    int16_t imu_heading;
    int16_t tgt_heading;
    int16_t cur_heading;
    int16_t drv_left;
    int16_t drv_right;
    int16_t weapon;
    int16_t gyro_corr;
    int16_t battery;
    int16_t rssi;
    int16_t loss_rate;
    int16_t packet_rate;
    int16_t rx_all;
    int16_t session_id;
}
PACK_STRUCT
detcord_runtime_log_t;

#endif
