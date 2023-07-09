#ifndef _NRF52DSHOT_H_
#define _NRF52DSHOT_H_

#include <Arduino.h>

#define NRFDSHOT_NUM_PULSES 17
//#define NRFDSHOT_ALWAYS_RECONFIG

enum
{
    DSHOT_SPEED_150, // not supported by AM32
    DSHOT_SPEED_300,
    DSHOT_SPEED_600,
    DSHOT_SPEED_1200, // not supported by AM32
};

class nRF52Dshot
{
    public:
        nRF52Dshot(int pin, uint8_t speed = DSHOT_SPEED_600, uint32_t interval = 10, NRF_PWM_Type* p_pwm = NULL, int8_t pwm_out = -1);
        void begin(void);
        void end(void);
        void task(void);
        void setThrottle(uint16_t t, bool sync);
        void sendCmd(uint16_t cmd, uint8_t cnt);
        void sendNow(void);
        void pwmConfig(void);

    private:
        int _pin; uint8_t _speed; uint32_t _interval, bool _active;
        NRF_PWM_Type* _pwm; int8_t _pwmout;
        uint32_t _period, _t0;
        uint32_t _last_time; // used for interval tracking
        uint16_t _throttle;  // store with checksum
        uint16_t _cmd;       // store with checksum
        uint8_t _cmdcnt = 0; // some commands need to be sent multiple times

        uint16_t _buffer[(NRFDSHOT_NUM_PULSES + 2)];
        bool _busy;
};

uint16_t nrfdshot_createPacket(uint16_t throttle);

#endif
