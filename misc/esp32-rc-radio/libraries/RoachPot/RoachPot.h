#ifndef _ROACHPOT_H_
#define _ROACHPOT_H_

#include <Arduino.h>
#include <RoachLib.h>

enum
{
    ROACHPOT_SM_NORMAL,
    ROACHPOT_SM_CALIB_CENTER,
    ROACHPOT_SM_CALIB_LIMITS,
    ROACHPOT_SM_IDLE,
    ROACHPOT_SM_CONVERTING,
};

class RoachPot
{
    public:
        RoachPot(int pin);
        void begin(void);
        int16_t get(void);
        bool task(void);
        void calib_center(void);
        void calib_limits(void);
        roach_nvm_pot_t cfg;
        bool has_new;
        bool calib_done;

    private:
        uint8_t state_machine;
        uint32_t calib_start_time;
        int _pin;
        int32_t last_adc, last_val;
        int32_t last_adc_filter;
        int32_t calib_sum, calib_cnt;
};

#endif
