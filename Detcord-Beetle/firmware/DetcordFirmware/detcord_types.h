#ifndef _DETCORD_TYPES_H_
#define _DETCORD_TYPES_H_

#include <RoachLib.h>

#include "detcord_cfg.h"
#include "detcord_defs.h"

typedef struct
{
    uint32_t magic;

    uint8_t  imu_orientation; // there are 6 x 8 combinations, see RoachImu's implementation

    roach_nvm_servo_t drive_left;
    bool              drive_left_flip;
    roach_nvm_servo_t drive_right;
    bool              drive_right_flip;
    roach_nvm_servo_t weapon;
    //roach_nvm_servo_t weapon2;

    roach_nvm_pid_t pid_heading;
    roach_nvm_pid_t pid_spindown;
    uint16_t spindown_limit;

    uint16_t heading_timeout;

    uint32_t checksum;
}
PACK_STRUCT
detcord_nvm_t;

#endif
