#include "nRF52Dshot.h"
#include <RoachLib.h>

nRF52Dshot::nRF52Dshot(int pin, uint32_t interval, uint8_t speed, NRF_PWM_Type* p_pwm, int8_t pwm_out)
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
            , NRF_PWM3
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
        #ifdef NRFDSHOT_SERIAL_ENABLE
        Serial.println("nRF52Dshot failed to init, no available PWM");
        #endif
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
        HwPWMx[pwmidx]->takeOwnership(0xABCD5101);
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
    #ifdef NRFDSHOT_SUPPORT_SPEED_1200
    else if (_speed == DSHOT_SPEED_1200)
    {
        // period is 0.83 us, t0 is 0.3125 us
        _period = 13;
        _t0 = 5;
    }
    #endif
    else
    {
        #ifdef NRFDSHOT_SERIAL_ENABLE
        Serial.printf("nRF52Dshot failed to init, unknown speed setting %u\r\n", _speed);
        #endif
    }

    pwmConfig();

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);

    #if defined(ARDUINO_ARCH_NRF52840)
        _pwm->PSEL.OUT[_pwmout] = g_APinDescription[_pin].name;
    #else
        _pwm->PSEL.OUT[_pwmout] = g_ADigitalPinMap[_pin];
    #endif

    _pwm->ENABLE = 1;
    _active = true;
}

void nRF52Dshot::detach(void)
{
    #if defined(ARDUINO_ARCH_NRF52840)
        _pwm->PSEL.OUT[_pwmout] = 0xFFFFFFFF;
    #else
        _pwm->PSEL.OUT[_pwmout] = 0xFFFFFFFF;
    #endif
    pinMode(_pin, INPUT);
    _active = false;
    _throttle = 0;
}

void nRF52Dshot::pwmConfig(void)
{
    _pwm->MODE       = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);                    // Set the wave mode to count UP
    _pwm->PRESCALER  = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos); // Set the PWM to use the 16MHz clock
    _pwm->COUNTERTOP = (_period << PWM_COUNTERTOP_COUNTERTOP_Pos);                     // Setting of the maximum count, 20 is 1.25us
    _pwm->LOOP       = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);                    // Disable loops, we want the sequence to repeat only once
    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word
    // of 16bits
    _pwm->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    _pwm->SEQ[_pwmout].PTR = (uint32_t)(_buffer) << PWM_SEQ_PTR_PTR_Pos;
    _pwm->SEQ[_pwmout].CNT = (NRFDSHOT_NUM_PULSES + NRFDSHOT_ZEROPAD) << PWM_SEQ_CNT_CNT_Pos;
    _pwm->SEQ[_pwmout].REFRESH = 0;
    _pwm->SEQ[_pwmout].ENDDELAY = 0;
}

void nRF52Dshot::setThrottle(uint16_t t, bool sync)
{
    _throttle = nrfdshot_createPacket(t + DSHOT_MIN_VALUE);
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
    uint16_t mask, i;
    for (mask = 0x8000, i = 0; i < 16; mask >>= 1, i++)
    {
        _buffer[i] = (_t0 * (((data & mask) != 0) ? 2 : 1)) | (0x8000);
    }
    #if defined(NRFDSHOT_ZEROPAD) && (NRFDSHOT_ZEROPAD >= 1)
    _buffer[i]     = 0 | (0x8000); // zero pad to end
    #endif
    #if defined(NRFDSHOT_ZEROPAD) && (NRFDSHOT_ZEROPAD >= 2)
    _buffer[i + 1] = 0 | (0x8000); // zero pad to end
    #endif

    #ifdef NRFDSHOT_ALWAYS_RECONFIG
    pwmConfig();
    #endif

    _pwm->EVENTS_SEQEND[_pwmout]  = 0UL;
    _pwm->TASKS_SEQSTART[_pwmout] = 1UL;
    _last_time = millis();
    #ifdef NRFDSHOT_BLOCKING
    while (!_pwm->EVENTS_SEQEND[_pwmout]) {
        yield();
    }
    #else
    _busy = true;
    #endif
}

void nRF52Dshot::writeMicroseconds(uint16_t us)
{
    #ifdef NRFDSHOT_SUPPORT_SPEED_1200
    setThrottle(convertPpm(us));
    #else
    setThrottle(nrfdshot_convertPpm(us));
    #endif
}

#define NRFDSHOT_CONV_THROTTLE(ppm) \
    const int32_t in_min  = ROACH_SERVO_MIN; \
    const int32_t in_max  = ROACH_SERVO_MAX; \
    const int32_t out_min = 0;      \
    int32_t a = (ppm)   - in_min;   \
    int32_t b = out_max - out_min;  \
    int32_t c = in_max  - in_min;   \
    int32_t d = a * b;              \
                                    \
    d += c / 2;                     \
    int32_t y = d / c;              \
                                    \
    y += out_min;                   \
                                    \
    if (y > out_max) {              \
        y = out_max;                \
    }                               \
    else if (y < out_min) {         \
        y = out_min;                \
    }                               \
    return y;                       \

uint16_t nRF52Dshot::convertPpm(uint16_t ppm)
{
    #ifndef NRFDSHOT_SUPPORT_SPEED_1200
    const
    #endif
    int32_t out_max = (
    #ifdef NRFDSHOT_SUPPORT_SPEED_1200
        (_speed == DSHOT_SPEED_1200) ? 4095 :
    #endif
        DSHOT_MAX_VALUE ) - DSHOT_MIN_VALUE;
    NRFDSHOT_CONV_THROTTLE(ppm);
}

uint16_t nrfdshot_convertPpm(uint16_t ppm)
{
    const int32_t out_max = DSHOT_MAX_VALUE;

    NRFDSHOT_CONV_THROTTLE(ppm);
}

// calculates the checksum required
uint16_t nrfdshot_createPacket(uint16_t throttle)
{
    uint8_t csum = 0;
    throttle <<= 1;

    // Indicate as command if less than 48
    if (throttle < DSHOT_MIN_VALUE && throttle > 0) {
        throttle |= 1;
    }
    else if (throttle > DSHOT_MAX_VALUE) { // limit throttle to max possible
        throttle = DSHOT_MAX_VALUE;
    }

    uint16_t csum_data = throttle;
    for (uint8_t i = 0; i < 3; i++){
        csum ^= csum_data;
        csum_data >>= 4;
    }
    csum &= 0x0F;
    return (throttle << 4) | csum;
}
