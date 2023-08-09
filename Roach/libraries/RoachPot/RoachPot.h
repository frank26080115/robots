#ifndef _ROACHPOT_H_
#define _ROACHPOT_H_

#include <Arduino.h>
#include <RoachLib.h>

#define POT_CNT_MAX 8

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
        RoachPot(int pin, roach_nvm_pot_t* c, uint32_t g = SAADC_CH_CONFIG_GAIN_Gain1_6, uint32_t r = SAADC_CH_CONFIG_REFSEL_Internal);
        void begin(void);
        int16_t get(void);
        int16_t getAdcFiltered(void);
        int16_t getAdcRaw(void);
        void task(void);
        void calib_center(void);
        void calib_limits(void);
        void calib_stop(void);
        void simulate(int x);
        void setFilter(float
        void prep(void);
        bool has_new;
        bool calib_done;
        roach_nvm_pot_t* cfg = NULL;
        int32_t alt_filter = 0;

    private:
        uint8_t state_machine;
        uint32_t calib_start_time;
        int _pin;
        int32_t last_adc, last_val, last_adc_raw;
        int32_t last_adc_filter;
        int32_t calib_sum, calib_cnt;
        int32_t simulate_val;
        uint32_t nrf_gain, nrf_ref;
};

void RoachPot_allTask(void);
void RoachPot_allBegin(void);
int16_t RoachPot_getRawAtIdx(int);

#endif
