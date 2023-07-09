#include "nRF52Dshot.h"

nRF52Dshot::nRF52Dshot(int pin, uint8_t speed, uint32_t interval, NRF_PWM_Type* p_pwm, int8_t pwm_out)
{
    _pin = pin;
    _speed = speed;
    _interval = interval;
    _pwm = p_pwm;
    _pwmout = pwm_out;
}

void nRF52Dshot::begin(void)
{
    if (_pwm == NULL)
    {
        // none specified, so look through all available PWM modules and see which one is free
        NRF_PWM_Type *PWM[] = {
            NRF_PWM0,
            NRF_PWM1,
            NRF_PWM2
        #if defined(NRF_PWM3)
            ,
            NRF_PWM3
        #endif
          };
        unsigned int i;
        for (i = 0; i < (sizeof(PWM) / sizeof(PWM[0])); i++)
        {
            if ((PWM[i]->ENABLE == 0) &&
                (PWM[i]->PSEL.OUT[0] & PWM_PSEL_OUT_CONNECT_Msk) &&
                (PWM[i]->PSEL.OUT[1] & PWM_PSEL_OUT_CONNECT_Msk) &&
                (PWM[i]->PSEL.OUT[2] & PWM_PSEL_OUT_CONNECT_Msk) &&
                (PWM[i]->PSEL.OUT[3] & PWM_PSEL_OUT_CONNECT_Msk))
            {
                _pwm = PWM[i];
                break;
            }
        }
        #ifdef NRFDSHOT_DEBUG
        if (_pwm != NULL) {
            Serial.printf("nRF52Dshot found PWM %u\r\n", i);
        }
        #endif
    }
    if (_pwmout < 0)
    {
        for (int i = 0; i <= 3; i++)
        {
            if (_pwm->PSEL.OUT[i] & PWM_PSEL_OUT_CONNECT_Msk) {
                _pwmout = i;
                break;
            }
        }
        #ifdef NRFDSHOT_DEBUG
        if (_pwmout >= 0) {
            Serial.printf("nRF52Dshot found PWM output idx %u\r\n", _pwmout);
        }
        #endif
    }

    if (_pwm == NULL || _pwmout < 0) {
        Serial.println("nRF52Dshot failed to init, no available PWM");
        return;
    }

    // Arduino uses HardwarePWM class, we can mark an instance as being "owned" to prevent it from being taken over later (by PWM or Servo)
    int pwmidx = -1;
    switch ((uint32_t)_pwm)
    {
        case (uint32_t)NRF_PWM0_BASE: pwmidx = 0; break;
        case (uint32_t)NRF_PWM1_BASE: pwmidx = 1; break;
        case (uint32_t)NRF_PWM2_BASE: pwmidx = 2; break;
        #ifdef NRF_PWM3
        case (uint32_t)NRF_PWM3_BASE: pwmidx = 3; break;
        #endif
    }
    if (pwmidx >= 0) {
        HwPWMx[pwmidx]->takeOwnership(0xABCD1ED5);
    }

    if (_speed == DSHOT_SPEED_150)
    {
        // period is 6.667 us, t0 is 2.5 us
        _period = 107;
        _t0 = 40;
    }
    else if (_speed == DSHOT_SPEED_300)
    {
        // period is 3.333 us, t0 is 1.25 us
        _period = 53;
        _t0 = 20;
    }
    else if (_speed == DSHOT_SPEED_600)
    {
        // period is 1.667 us, t0 is 0.625 us
        _period = 27;
        _t0 = 10;
    }
    else if (_speed == DSHOT_SPEED_1200)
    {
        // period is 0.83 us, t0 is 0.3125 us
        _period = 13;
        _t0 = 5;
    }

    pwmConfig();

    pinMode(_pind, OUTPUT);
    digitalWrite(_pind, LOW);

    #if defined(ARDUINO_ARCH_NRF52840)
        _pwm->PSEL.OUT[_pwmout] = g_APinDescription[_pind].name;
    #else
        _pwm->PSEL.OUT[_pwmout] = g_ADigitalPinMap[_pind];
    #endif

    _pwm->ENABLE = 1;
    _active = true;
}

void nRF52Dshot::end(void)
{
    #if defined(ARDUINO_ARCH_NRF52840)
        _pwm->PSEL.OUT[_pwmout] = g_APinDescription[_pind].name;
    #else
        _pwm->PSEL.OUT[_pwmout] = g_ADigitalPinMap[_pind];
    #endif
    pinMode(_pind, INPUT);
    _active = false;
}

void nRF52Dshot::pwmConfig(void)
{
    _pwm->MODE       = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);                    // Set the wave mode to count UP
    _pwm->PRESCALER  = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos); // Set the PWM to use the 16MHz clock
    _pwm->COUNTERTOP = (_period << PWM_COUNTERTOP_COUNTERTOP_Pos);                     // Setting of the maximum count, 20 is 1.25us
    _pwm->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);                          // Disable loops, we want the sequence to repeat only once
    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word
    // of 16bits
    _pwm->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    _pwm->SEQ[_pwmout].PTR = (uint32_t)(_buffer) << PWM_SEQ_PTR_PTR_Pos;
    _pwm->SEQ[_pwmout].CNT = ((NRFDSHOT_NUM_PULSES + 2) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos;
    _pwm->SEQ[_pwmout].REFRESH = 0;
    _pwm->SEQ[_pwmout].ENDDELAY = 0;
}

void nRF52Dshot::setThrottle(uint16_t t, bool sync)
{
    _throttle = nrfdshot_createPacket(t);
    if (sync)
    {
        _cmdcnt = 0;
        sendNow();
    }
}

void nRF52Dshot::sendCmd(uint16_t cmd, uint8_t cnt)
{
    _cmd = nrfdshot_createPacket(cmd);
    _cmdcnt = cnt;
    sendNow();
}

void nRF52Dshot::task(void)
{
    if (_active == false) {
        return;
    }
    uint32_t now = millis();
    if ((now - _last_time) >= _interval)
    {
        sendNow();
    }
}

void nRF52Dshot::sendNow(void)
{
    // if non-blocking mode, then we need to wait until previous sequence has finished
    if (_busy)
    {
        while (!_pwm->EVENTS_SEQEND[_pwmout]) {
            yield();
        }
    }
    _busy = false;

    _last_time = millis();
    #ifdef NRFDSHOT_ALWAYS_RECONFIG
    pwmConfig();
    #endif
    uint16_t data;
    if (_cmdcnt > 0)
    {
        data = _cmd;
        _cmdcnt--;
    }
    else
    {
        data = _throttle;
    }
    uint8_t i;
    for (i = 0; i < 16; i++)
    {
        _buffer[i] = (_t0 * (((data & (1 << (16 - i - 1))) != 0) ? 2 : 1)) | (0x8000);
    }
    buffer[i]     = 0 | (0x8000); // zero pad to end
    buffer[i + 1] = 0 | (0x8000); // zero pad to end

    #ifdef ROACHNEOPIX_ALWAYS_RECONFIG
    pwmConfig();
    #endif

    _pwm->EVENTS_SEQEND[_pwmout]  = 0UL;
    _pwm->TASKS_SEQSTART[_pwmout] = 1UL;
    _busy = true;
}

// calculates the checksum required
uint16_t nrfdshot_createPacket(uint16_t throttle)
{
    uint8_t csum = 0;
    throttle <<= 1;

    // Indicate as command if less than 48
    if (throttle < 48 && throttle > 0) {
        throttle |= 1;
    }

    uint16_t csum_data = throttle;
    for (uint8_t i = 0; i < 3; i++){
        csum ^= csum_data;
        csum_data >>= 4;
    }
    csum &= 0x0F;
    return (throttle << 4) | csum;
}