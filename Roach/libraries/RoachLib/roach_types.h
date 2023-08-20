#ifndef _ROACH_TYPES_H_
#define _ROACH_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#include "roach_conf.h"
#include "roach_defs.h"

#define PACK_STRUCT __attribute__ ((packed))

enum
{
    ROACHPKTFLAG_BTN1 = 0x01,
    ROACHPKTFLAG_BTN2 = 0x02,
    ROACHPKTFLAG_BTN3 = 0x04,
    ROACHPKTFLAG_BTN4 = 0x08,
    ROACHPKTFLAG_SAFE = 0x80,
};

enum
{
    ENCODERMODE_NORMAL,
    ENCODERMODE_USEPOT,
};

enum
{
    ROACH_FLIP_NONE,
    ROACH_FLIP_LR,
    ROACH_FLIP_REV_RIGHT,
    ROACH_FLIP_REV_LEFT,
};

enum
{
    ROACHCMD_TEXT = '!',
    ROACHCMD_SYNC_DOWNLOAD_DESC = 'D',
    ROACHCMD_SYNC_DOWNLOAD_CONF = 'd',
    ROACHCMD_SYNC_UPLOAD_CONF   = 'C',
};

// try to keep this under 32 bytes
typedef struct
{
    int16_t  throttle; // range +/- ROACH_SCALE_MULTIPLIER
    int16_t  steering; // range +/- ROACH_SCALE_MULTIPLIER
    int16_t  heading;  // absolute encoder data to set absolute heading, range -1800 to 1800 (deg * ROACH_ANGLE_MULTIPLIER)
    int16_t  pot_weap; // range 0 to ROACH_SCALE_MULTIPLIER
    int16_t  pot_aux;  // range 0 to ROACH_SCALE_MULTIPLIER
    uint32_t flags;    // ROACHPKTFLAG_*
}
PACK_STRUCT
roach_ctrl_pkt_t;

typedef struct
{
    uint16_t timestamp;
    uint32_t chksum_desc; // checksum over the NVM description structure, used as a UUID
    uint32_t chksum_nvm;  // checksum over the NVM contents, used to detect changes
    int16_t  heading;     // unit -1800 to 1800 (deg * ROACH_ANGLE_MULTIPLIER)
    uint16_t battery;     // unit mv, expected 0 to 4200
    uint8_t  temperature; // uhhhhh, not used right now
    int8_t   rssi;        // 16 is the best it can do, higher numer is worse
    uint16_t loss_rate;   // per second packet loss rate
}
PACK_STRUCT
roach_telem_pkt_t;

#define ROACH_HEADING_INVALID_NOTREADY  0xFFFF
#define ROACH_HEADING_INVALID_HASFAILED 0xFFFE

typedef struct
{
    uint32_t uid;      // unique ID for one link
    uint32_t salt;     // secret, never exposed
    uint32_t chan_map; // each bit represents a channel
}
PACK_STRUCT
roach_rf_nvm_t;

typedef struct
{
    int16_t center;    // microsecond units, should be 1500
    int16_t deadzone;  // microsecond units
    int32_t trim;      // microsecond units
    int16_t scale;     // scaled with ROACH_SCALE_MULTIPLIER = 1
    int16_t limit_max; // microsecond units, should be 2000
    int16_t limit_min; // microsecond units, should be 1000
}
PACK_STRUCT
roach_nvm_servo_t;

// PID output expected to be +/- ROACH_SCALE_MULTIPLIER^2, used with RoachDriveMixer gyro_correction
// ROACH_SCALE_MULTIPLIER^2 = 1000 * 1000 = 1000000
// ROACH_ANGLE_MULTIPLIER = 10
typedef struct
{
    int32_t  p; // default estimate: 50 degree means 500 error, output needs to be ROACH_SCALE_MULTIPLIER^2
                // p = ROACH_SCALE_MULTIPLIER^2 / 500
    int32_t  i; // default estimate: error accumulated of 180000, divided by ROACH_ANGLE_MULTIPLIER internally
                // i = ROACH_SCALE_MULTIPLIER^2 / 18000
    int32_t  d; // default estimate: 100 degree correction in one second means delta = 100
                // d = ROACH_SCALE_MULTIPLIER^2 / 100
                // note: d term polarity automatically determined
    uint32_t output_limit;      // ROACH_SCALE_MULTIPLIER^2
    uint32_t accumulator_limit; // upper limit of the error accumulator // error of 1800 collected over 1 seconds is 180000
    uint32_t accumulator_decay; // decay of accumulator once per tick (10ms usually)
}
PACK_STRUCT
roach_nvm_pid_t;

typedef struct
{
    int32_t limit_rot;
    int32_t limit_diff;
    int32_t accel;
    int32_t deccel;
}
PACK_STRUCT
roach_nvm_virheading_t;

typedef struct
{
    int16_t center;    // this is a ADC value, calibrated
    int16_t deadzone;  // ADC scale
    int16_t limit_max; // ADC scale, calibrated
    int16_t limit_min; // ADC scale, calibrated
    int16_t boundary;  // ADC scale
    int16_t expo;      // 0 = straight line, ROACH_SCALE_MULTIPLIER = exponential
    int16_t filter;    // 0 to ROACH_SCALE_MULTIPLIER, 1 meaning heavy filter, ROACH_SCALE_MULTIPLIER means full pass
    int16_t scale;     // multiplier, scaled with ROACH_SCALE_MULTIPLIER, default should be ROACH_SCALE_MULTIPLIER = 1
}
PACK_STRUCT
roach_nvm_pot_t;

typedef struct
{
    uint32_t magic;

    uint8_t startup_switches;      // what the start-up switch positions need to be to unlock
    uint8_t startup_switches_mask; // which bits we care about

    roach_nvm_pot_t pot_throttle;
    roach_nvm_pot_t pot_steering;
    int32_t heading_multiplier;   // need to multiply encoder ticks into heading angle, range -1800 to 1800 (deg * ROACH_ANGLE_MULTIPLIER), for 400 tick encoder, 3600 / (400 * 4) = 

    roach_nvm_pot_t pot_weapon;
    roach_nvm_pot_t pot_aux;
    roach_nvm_pot_t pot_battery;

    uint32_t checksum;
}
PACK_STRUCT
roach_tx_nvm_t;

typedef struct
{
    uint32_t byte_offset;
    char     name[32];
    char     type_code[16]; // "u8", "s8", "u16", "s16", "u32", "s32", "s32x100", "hex"
    int32_t  def_val;
    int32_t  limit_min;     // always populate
    int32_t  limit_max;     // always populate
    int32_t  step;          // 0 for disable
}
roach_nvm_gui_desc_t;

#define ROACH_NVM_GUI_DESC_END    { 0, "\0", "\0", 0, 0, 0, 0, }

#endif
