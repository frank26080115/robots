#include "RoachPot.h"

#define POT_CNT_MAX 8
static int pot_cnt = 0;
static RoachPot* pot_inst[POT_CNT_MAX];
static int pot_pin[POT_CNT_MAX];
static int pot_task_idx = 0;
static uint8_t adc_state_machine = ROACHPOT_SM_IDLE;

RoachPot::RoachPot(int pin, roach_nvm_pot_t* c)
{
    _pin = pin;
    _cfg = c;
    pot_inst[pot_cnt] = this;
    pot_pin[pot_cnt] = _pin;
    pot_cnt++;
}

void RoachPot::begin(void)
{
    adcAttachPin(_pin);
    state_machine = ROACHPOT_SM_NORMAL;
    has_new = false;
    calib_done = false;
}

int16_t RoachPot_task(int p)
{
    int16_t ret = -1;
    int cur_pin = pot_pin[pot_task_idx];
    if (p != cur_pin) {
        return ret;
    }
    if (adc_state_machine == ROACHPOT_SM_IDLE)
    {
        adcStart(cur_pin);
        adc_state_machine = ROACHPOT_SM_WAIT;
    }
    else if (adc_state_machine == ROACHPOT_SM_CONVERTING)
    {
        if (adcBusy(cur_pin) == false)
        {
            int16_t x = adcEnd(cur_pin);
            ret = x;
            pot_task_idx = (pot_task_idx + 1) % pot_cnt;
            adcStart(pot_pin[pot_task_idx]);
        }
    }
    return ret;
}

void RoachPot::task(void)
{
    int16_t x = RoachPot_task(_pin);
    if (x < 0) {
        return;
    }
    has_new = true;
    int32_t x32 = x;
    last_adc_raw = x32;
    if (cfg->filter != 0)
    {
        int32_t filtered = roach_lpf(x32, last_val_filter, cfg->filter);
        last_adc_filter = filtered;
        last_adc = roach_reduce_to_scale(filtered);
    }
    else
    {
        last_adc = x32;
    }

    x32 = last_adc;
    if (state_machine == ROACHPOT_SM_CALIB_CENTER)
    {
        calib_sum += last_val;
        calib_cnt += 1;
        if ((millis() - calib_start_time) >= 500)
        {
            int32_t avg = calib_sum + (calib_cnt / 2);
            avg /= calib_cnt;
            cfg->center = avg;
            state_machine = ROACHPOT_SM_NORMAL;
            calib_done = true;
        }
        last_val = 0;
    }
    else
    {
        if (state_machine == ROACHPOT_SM_CALIB_LIMITS)
        {
            cfg->limit_max = (x32 > cfg->limit_max) ? x32 : cfg->limit_max; 
            cfg->limit_min = (x32 < cfg->limit_min) ? x32 : cfg->limit_min;
            //if ((millis() - calib_start_time) >= 5000)
            //{
            //    state_machine = ROACHPOT_SM_NORMAL;
            //    calib_done = true;
            //}
        }

        x32 -= cfg->center;
        if (x32 > 0)
        {
            if (x32 < cfg->deadzone)
            {
                last_val = 0;
            }
            else
            {
                last_val = roach_value_map(x32 - cfg->deadzone, 0, cfg->limit_max - cfg->center - cfg->deadzone - cfg->boundary, 0, ROACH_SCALE_MULTIPLIER, true);
            }
        }
        else if (x32 < 0)
        {
            if (x32 > -cfg->deadzone)
            {
                last_val = 0;
            }
            else
            {
                x32 *= -1;
                last_val = -roach_value_map(x32 - cfg->deadzone, 0, cfg->center - cfg->limit_min - cfg->deadzone - cfg->boundary, 0, ROACH_SCALE_MULTIPLIER, true);
            }
        }
        else
        {
            last_val = 0;
        }

        if (cfg->curve != 0)
        {
            last_val = roach_expo_curve32(last_val, cfg->curve);
        }
    }
}

void RoachPot_allTask(void)
{
    int i;
    for (i = 0; i < pot_cnt; i++)
    {
        pot_inst[i]->task();
    }
}

void RoachPot_allBegin(void)
{
    int i;
    for (i = 0; i < pot_cnt; i++)
    {
        pot_inst[i]->begin();
    }
}

int16_t RoachPot::get(void)
{
    return last_val;
}

int16_t RoachPot::getAdcFiltered(void)
{
    return last_adc;
}

int16_t RoachPot::getAdcRaw(void)
{
    return last_adc_raw;
}

void RoachPot::calib_center(void)
{
    calib_sum = 0;
    calib_cnt = 0;
    state_machine = ROACHPOT_SM_CALIB_CENTER;
    calib_start_time = millis();
}

void RoachPot::calib_limits(void)
{
    cfg->limit_max -= cfg->boundary;
    cfg->limit_min += cfg->boundary;
    state_machine = ROACHPOT_SM_CALIB_LIMITS;
    calib_start_time = millis();
}

void RoachPot::calib_stop(void)
{
    state_machine = ROACHPOT_SM_NORMAL;
    calib_done = true;
}
