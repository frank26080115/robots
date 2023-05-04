#ifndef _ROACH_TYPES_H_
#define _ROACH_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#include "roach_conf.h"
#include "roach_defs.h"

#define PACK_STRUCT __attribute__ ((packed))

// try to keep this under 32 bytes
typedef struct
{
    // uint8_t  seq;      // sequence number, not required, as the internal protocol already carries one
    // uint8_t  pkt_type; // packet type, but we have a dedicated text channel for critical-but-not-frequent messages, we don't need this yet
    int16_t  throttle;
    int16_t  steering;
    int16_t  heading;  // absolute encoder data to set absolute heading
    int8_t   weap_speed;
    uint32_t flags;
    uint16_t chan_map; // the transmitter channel map might not be the same as the receiver, the receiver might be running in single channel mode
}
PACK_STRUCT
roach_ctrl_pkt_t;

typedef struct
{
    uint16_t timestamp;
    uint16_t battery;
    uint8_t  temperature;
    int16_t  rssi;
    int16_t  data_rate;
    int16_t  rxall_rate;
    int16_t  loss_rate;
}
PACK_STRUCT
roach_telem_pkt_t;

typedef struct
{
    uint32_t uid;
    uint32_t salt;
    uint32_t chan_map;
}
PACK_STRUCT
roach_rf_nvm_t;

typedef struct
{
    int16_t center;
    int16_t deadzone;
    int32_t trim;
    int16_t scale;
    int16_t limit_max;
    int16_t limit_min;
}
PACK_STRUCT
roach_nvm_servo_t;

typedef struct
{
    int32_t  p;
    int32_t  i;
    int32_t  d;
    uint32_t output_limit;
    uint32_t accumulator_limit;
    uint32_t accumulator_decay;
}
PACK_STRUCT
roach_nvm_pid_t;

typedef struct
{
    int16_t center;
    int16_t deadzone;
    int16_t limit_max;
    int16_t limit_min;
    int16_t boundary;
    int16_t expo;
    int16_t filter;
    int16_t scale;
}
PACK_STRUCT
roach_nvm_pot_t;

typedef struct
{
    uint8_t  imu_orientation; // there are 6 x 8 combinations, see RoachImu's implementation

    roach_nvm_servo_t drive_left;
    bool              drive_left_flip;
    roach_nvm_servo_t drive_right;
    bool              drive_right_flip;
    roach_nvm_servo_t weapon;

    roach_nvm_pid_t pid_heading;
    uint16_t heading_timeout;
}
PACK_STRUCT
roach_rx_nvm_t;

typedef struct
{
    roach_nvm_pot_t pot_throttle;
    roach_nvm_pot_t pot_steering;
    int32_t heading_multiplier;
    int32_t cross_mix;
    roach_nvm_pot_t pot_weapon;
}
PACK_STRUCT
roach_tx_nvm_t;

typedef struct
{
    uint32_t byte_offset;
    char     name[32];
    char     type_code[32]; // "u8", "s8", "u16", "s16", "u32", "s32", "s32x100", "hex"
    int32_t  def_val;
    int32_t  limit_min;     // always populate
    int32_t  limit_max;     // always populate
    int32_t  step;          // 0 for disable
}
roach_nvm_gui_desc_t;

#endif
