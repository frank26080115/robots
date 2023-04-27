#include "RoachRotaryEncoder.h"

void IRAM_ATTR RoachEnc_isr(void);
void IRAM_ATTR RoachEnc_task(void);

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
    enc_flags = 0;
    attachInterrupt(enc_pin_a, RoachEnc_isr, CHANGE);
    attachInterrupt(enc_pin_b, RoachEnc_isr, CHANGE);
    RoachEnc_task();
    enc_cnt = 0;
    enc_prev_action = 0;
    enc_time = millis();
}

void IRAM_ATTR RoachEnc_isr(void)
{
    RoachEnc_task();
}

void IRAM_ATTR RoachEnc_task(void)
{
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
            else if ((enc_flags & (1 << 2)) != 0) && ((enc_flags & (1 << 0)) != 0) || (enc_flags & (1 << 4)) != 0))) {
                action = 1;
            }
            else if ((enc_flags & (1 << 1)) != 0) && ((enc_flags & (1 << 3)) != 0) || (enc_flags & (1 << 4)) != 0))) {
                action = -1;
            }
            else if ((enc_flags & (1 << 3)) != 0) && ((enc_flags & (1 << 1)) != 0) || (enc_flags & (1 << 4)) != 0))) {
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
