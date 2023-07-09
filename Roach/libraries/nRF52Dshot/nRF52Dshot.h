#ifndef _NRF52DSHOT_H_
#define _NRF52DSHOT_H_

#include <Arduino.h>

//#define NRFDSHOT_SERIAL_ENABLE

#define NRFDSHOT_NUM_PULSES 16
//#define NRFDSHOT_ALWAYS_RECONFIG // use this if the PWM generator is shared
//#define NRFDSHOT_BLOCKING
#define NRFDSHOT_ZEROPAD 2
//#define NRFDSHOT_SUPPORT_SPEED_1200

enum
{
    DSHOT_SPEED_150,  // not supported by AM32
    DSHOT_SPEED_300,
    DSHOT_SPEED_600,
    #ifdef NRFDSHOT_SUPPORT_SPEED_1200
    DSHOT_SPEED_1200, // not supported by AM32
    #endif
};

enum
{
    DSHOTCMD_AM32_PLAYINPUTTUNE1  =  1,
    DSHOTCMD_AM32_PLAYINPUTTUNE2  =  2,
    DSHOTCMD_AM32_PLAYBEACONTUNE3 =  3,
    DSHOTCMD_AM32_PLAYSTARTUPTUNE =  5,
    DSHOTCMD_AM32_FLIP_NORMAL     =  7,
    DSHOTCMD_AM32_FLIP_REVERSE    =  8,
    DSHOTCMD_AM32_BIDIR_DISABLE   =  9,
    DSHOTCMD_AM32_BIDIR_ENABLE    = 10,
    DSHOTCMD_AM32_SAVE_EEPROM     = 12,
    DSHOTCMD_AM32_TELEM_ENABLE    = 13,
    DSHOTCMD_AM32_TELEM_DISABLE   = 14,
    DSHOTCMD_AM32_DIR_NORMAL      = 20,
    DSHOTCMD_AM32_DIR_REVERSE     = 21,
};

class nRF52Dshot
{
    public:
        nRF52Dshot(int pin, uint8_t speed = DSHOT_SPEED_300, uint32_t interval = 10, NRF_PWM_Type* p_pwm = NULL, int8_t pwm_out = -1);
        void begin(void);
        void end(bool prep_bootload);                     // release pin, to be used for serial port later
        void task(void);                                  // automatically sends packets at regular intervals
        void setThrottle(uint16_t t, bool sync = false);  // sets the throttle value to be sent periodically
        void sendCmd(uint16_t cmd, uint8_t cnt = 1);      // sends a DSHOT command, optionally repeat
        void sendNow(void);                               // send the next packet right now;
        void pwmConfig(void);                             // configures the PWM generator
        uint16_t convertPpm(uint16_t ppm);                // convert a 1000-2000 PPM value to 0 to 2047 for DSHOT (or 0 to 4095 for DSHOT1200)

    private:
        int _pin; uint8_t _speed; uint32_t _interval; bool _active;
        NRF_PWM_Type* _pwm; int8_t _pwmout;
        uint32_t _period, _t0; // PWM timer parameters
        uint32_t _last_time;   // used for interval tracking
        uint16_t _throttle;    // cached value, store with checksum
        uint16_t _cmd;         // cached value, store with checksum
        uint8_t  _cmdcnt = 0;  // some commands need to be sent multiple times

        uint16_t _buffer[(NRFDSHOT_NUM_PULSES + NRFDSHOT_ZEROPAD)]; // stores the pulse-train for the DMA
        bool _busy;
};

uint16_t nrfdshot_convertPpm(uint16_t ppm);        // convert a 1000-2000 PPM value to 0 to 2047 for DSHOT
uint16_t nrfdshot_createPacket(uint16_t throttle); // creates the DSHOT packet with CRC

#endif
