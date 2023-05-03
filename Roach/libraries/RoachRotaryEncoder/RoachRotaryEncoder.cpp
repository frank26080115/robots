#include "RoachRotaryEncoder.h"

#if defined(ROACHENC_USE_GPIO)
void IRAM_ATTR RoachEnc_isr(void);
void IRAM_ATTR RoachEnc_task(void);
#elif defined(ROACHENC_USE_QDEC)
#include "nrf_qdec.h"
void RoachEnc_task(void);
void RoachEnc_isr(nrf_qdec_event_t event);
#endif

static volatile int32_t enc_cnt;
static volatile uint8_t enc_prev, enc_flags;
static volatile int8_t  enc_prev_action;
static volatile bool enc_moved;
static volatile uint32_t enc_time;
static int enc_pin_a, enc_pin_b;

void RoachEnc_begin(int pin_a, int pin_b)
{
    pinMode(enc_pin_a = pin_a, INPUT_PULLUP);
    pinMode(enc_pin_b = pin_b, INPUT_PULLUP);

#if defined(ROACHENC_USE_GPIO)
    enc_flags = 0;
    attachInterrupt(enc_pin_a, RoachEnc_isr, CHANGE);
    attachInterrupt(enc_pin_b, RoachEnc_isr, CHANGE);
#elif defined(ROACHENC_USE_QDEC)
    nrf_qdec_sampleper_set(NRF_QDEC_SAMPLEPER_128us);
    nrf_gpio_cfg_input((uint32_t)enc_pin_a, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input((uint32_t)enc_pin_b, NRF_GPIO_PIN_PULLUP);
    nrf_qdec_pio_assign((uint32_t)enc_pin_a, (uint32_t)enc_pin_b, NRF_QDEC_LED_NOT_CONNECTED);
    nrf_qdec_dbfen_enable();
    nrf_qdec_enable();
    nrf_qdec_task_trigger(NRF_QDEC_TASK_START);
#endif
    RoachEnc_task();
    enc_cnt = 0;
    enc_prev_action = 0;
    enc_time = millis();
}

#if defined(ROACHENC_USE_GPIO)
void IRAM_ATTR RoachEnc_isr(void)
#elif defined(ROACHENC_USE_QDEC)
void RoachEnc_isr(nrf_qdec_event_t event)
#endif
{
    RoachEnc_task();
}

void
#if defined(ROACHENC_USE_GPIO) && defined(ESP32)
IRAM_ATTR
#endif
RoachEnc_task(void)
{
    #if defined(ROACHENC_USE_GPIO)
    int8_t action = 0; // 1 or -1 if moved, sign is direction
    uint8_t cur_pos = 0;
    if (digitalRead(enc_pin_a) == LOW) {
        cur_pos |= (1 << 0);
    }
    if (digitalRead(enc_pin_b) == LOW) {
        cur_pos |= (1 << 1);
    }

    if (cur_pos != enc_prev) // any rotation at all
    {
        #ifdef ROACHENC_MODE_COMPLEX
        if (enc_prev == 0x00)
        {
            // first edge
            if (cur_pos == 0x01)
            {
                enc_flags |= (1 << 0);
            }
            else if (cur_pos == 0x02)
            {
                enc_flags |= (1 << 1);
            }
        }

        if (cur_pos == 0x03) // mid-step
        {
            enc_flags |= (1 << 4);
        }
        else if (cur_pos == 0x00)
        {
            // final edge
            if (enc_prev == 0x02)
            {
                enc_flags |= (1 << 2);
            }
            else if (enc_prev == 0x01)
            {
                enc_flags |= (1 << 3);
            }

            // check the first and last edge
            // or maybe one edge is missing, if missing then require the middle state
            // this will reject bounces and false movements
            if ((enc_flags & (1 << 0)) != 0 && ((enc_flags & (1 << 2)) != 0 || (enc_flags & (1 << 4)) != 0)) {
                action = 1;
            }
            else if ((enc_flags & (1 << 2)) != 0 && ((enc_flags & (1 << 0)) != 0 || (enc_flags & (1 << 4)) != 0)) {
                action = 1;
            }
            else if ((enc_flags & (1 << 1)) != 0 && ((enc_flags & (1 << 3)) != 0 || (enc_flags & (1 << 4)) != 0)) {
                action = -1;
            }
            else if ((enc_flags & (1 << 3)) != 0 && ((enc_flags & (1 << 1)) != 0 || (enc_flags & (1 << 4)) != 0)) {
                action = -1;
            }
        }
        #else
        action = enc_prev_action;
        switch (cur_pos)
        {
            case 0x00:
                switch (enc_prev)
                {
                    case 0x02: action =  1; break;
                    case 0x01: action = -1; break;
                }
                break;
            case 0x01:
                switch (enc_prev)
                {
                    case 0x00: action =  1; break;
                    case 0x03: action = -1; break;
                }
                break;
            case 0x02:
                switch (enc_prev)
                {
                    case 0x03: action =  1; break;
                    case 0x00: action = -1; break;
                }
                break;
            case 0x03:
                switch (enc_prev)
                {
                    case 0x01: action =  1; break;
                    case 0x02: action = -1; break;
                }
                break;
        }
        #endif

        if (action != 0)
        {
            enc_time = millis();
            enc_prev_action = action;
            enc_moved = true;
            enc_cnt += action;
        }
        enc_prev = cur_pos;
    }
    else
    {
        if ((millis() - enc_time) >= 1000)
        {
            enc_prev_action = 0;
        }
    }
    #elif defined(ROACHENC_USE_QDEC)
    uint32_t now = millis();
    NRF_QDEC->TASKS_READCLRACC = 1;
    int32_t x = NRF_QDEC->ACCREAD;
    if (x != 0) {
        enc_moved |= true;
        enc_time = now;
        enc_cnt += x;
        //enc_prev_action = x > 0 ? 1 : -1
    }
    //else if ((now - enc_time) >= 1000)
    //{
    //    enc_prev_action = 0;
    //}
    #endif
}

int32_t RoachEnc_get(bool clr)
{
    int32_t x = enc_cnt;
    if (clr) {
        enc_cnt = 0;
    }
    return enc_cnt;
}

bool RoachEnc_hasMoved(bool clr)
{
    bool x = enc_moved;
    if (clr) {
        enc_moved = false;
    }
    return x;
}

uint32_t RoachEnc_getLastTime(void)
{
    return enc_time;
}
