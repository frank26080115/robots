#include "RoachHeartbeat.h"
#include "RoachHeartbeat.h"
#include "nrf_wdt.h"
#include "wiring_private.h"

void RoachWdt_init(uint32_t tmr_ms)
{
    // calculate CRV value for millisecond units
    volatile uint32_t x = tmr_ms;
    x *= 32768;
    x /= 1000;

    NRF_WDT->CONFIG = WDT_CONFIG_SLEEP_Msk | WDT_CONFIG_HALT_Msk; // WDT run in all situations
    NRF_WDT->CRV = x;           // set the timeout
    NRF_WDT->TASKS_START = 1UL; // start WDT
    RoachWdt_feed();
}

void RoachWdt_feed(void)
{
    NRF_WDT->RR[0] = NRF_WDT_RR_VALUE;
}

RoachRgbLed::RoachRgbLed(bool dotstar, int pind, int pinc, NRF_SPIM_Type* p_spi)
{
    _spi = p_spi;
    _dotstar = dotstar;
    _pind = pind;
    _pinc = _dotstar ? pinc : RHB_HW_PIN_NEOPIXEL_UNUSED;

    _spim.p_reg = p_spi;

    #if NRFX_SPIM0_ENABLED
    if ( NRF_SPIM0 == p_spi ) {
        _spim.drv_inst_idx = NRFX_SPIM0_INST_IDX;
    }
    #endif

    #if NRFX_SPIM1_ENABLED
    if ( NRF_SPIM1 == p_spi ) {
        _spim.drv_inst_idx = NRFX_SPIM1_INST_IDX;
    }
    #endif

    #if NRFX_SPIM2_ENABLED
    if ( NRF_SPIM2 == p_spi ) {
        _spim.drv_inst_idx = NRFX_SPIM2_INST_IDX;
    }
    #endif

    #if NRFX_SPIM3_ENABLED
    if ( NRF_SPIM3 == p_spi ) {
        _spim.drv_inst_idx = NRFX_SPIM3_INST_IDX;
    }
    #endif
}

RoachNeoPixel::RoachNeoPixel(int pind, NRF_PWM_Type* p_pwm, int pwm_out)
{
    _pwm = p_pwm;
    _pind = pind;
    _pwmout = pwm_out;
}

void RoachRgbLed::begin(void)
{
    spiConfig();
}

void RoachNeoPixel::begin(void)
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
        #ifdef ROACHNEOPIX_DEBUG
        if (_pwm != NULL) {
            Serial.printf("neopix found PWM %u\r\n", i);
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
        #ifdef ROACHNEOPIX_DEBUG
        if (_pwmout >= 0) {
            Serial.printf("neopix found PWM output idx %u\r\n", _pwmout);
        }
        #endif
    }

    if (_pwm == NULL || _pwmout < 0) {
        Serial.println("RoachNeoPixel failed to init, no available PWM");
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

    pwmConfig();

    pinMode(_pind, OUTPUT);
    digitalWrite(_pind, LOW);

    #if defined(ARDUINO_ARCH_NRF52840)
        _pwm->PSEL.OUT[_pwmout] = g_APinDescription[_pind].name;
    #else
        _pwm->PSEL.OUT[_pwmout] = g_ADigitalPinMap[_pind];
    #endif

    _pwm->ENABLE = 1;
}

void RoachNeoPixel::pwmConfig(void)
{
    _pwm->MODE       = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);                    // Set the wave mode to count UP
    _pwm->PRESCALER  = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos); // Set the PWM to use the 16MHz clock
    _pwm->COUNTERTOP = (20UL << PWM_COUNTERTOP_COUNTERTOP_Pos);                        // Setting of the maximum count, 20 is 1.25us
    _pwm->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);                          // Disable loops, we want the sequence to repeat only once
    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word
    // of 16bits
    _pwm->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    _pwm->SEQ[_pwmout].PTR = (uint32_t)(_pattern) << PWM_SEQ_PTR_PTR_Pos;
    _pwm->SEQ[_pwmout].CNT = (ROACHNEOPIX_BUFFER_SIZE8 / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos;
    _pwm->SEQ[_pwmout].REFRESH = 0;
    _pwm->SEQ[_pwmout].ENDDELAY = 0;
}

#ifndef ROACHRGBLED_BLOCKING
static volatile bool spi_busy = false;
static void spi_handler(nrfx_spim_evt_t const * p_event, void * p_context)
{
    // there's nothing to check, nrfx_spim_evt_type_t literally only has one "done" enumeration, no other events are possible
    spi_busy = false;
}
#endif

void RoachRgbLed::set(uint8_t r, uint8_t g, uint8_t b, uint8_t brite, bool force)
{
    if (r == _r && g == _g && b == _b && brite == _brite && force == false)
    {
        // don't waste time setting the same colour
        return;
    }

    #ifndef ROACHRGBLED_BLOCKING
    // if non-blocking mode, then we need to wait until previous transaction has finished
    while (spi_busy) {
        yield();
    }
    #endif

    _r = r;
    _g = g;
    _b = b;
    _brite = brite;

    uint16_t len;

    if (_dotstar)
    {
        // prepare buffer according to packet format
        _buffer[ 0] = 0x00;
        _buffer[ 1] = 0x00;
        _buffer[ 2] = 0x00;
        _buffer[ 3] = 0x00;
        _buffer[ 4] = (brite >> 3) | 0xE0; 
        _buffer[ 5] = b;
        _buffer[ 6] = g;
        _buffer[ 7] = r;
        _buffer[ 8] = 0xFF;
        _buffer[ 9] = 0xFF;
        _buffer[10] = 0xFF;
        _buffer[11] = 0xFF;
        len = 12;
        #ifdef ROACHRGBLED_DEBUG
        Serial.printf("dotstar buff [%u]: ", len);
        int k;
        for (k = 0; k < len; k++) {
            Serial.printf("%02X ", _buffer[k]);
        }
        Serial.printf("\r\n");
        #endif
    }
    else
    {
            // no real brightness control, except for OFF
        if (brite <= 0)
        {
            r = 0;
            g = 0;
            b = 0;
        }

        memset(_buffer, 0, ROACHRGBLED_BUFFER_SIZE);
        uint8_t buf[3] = { g, r, b, }; // define byte order the LED expects
        int i, bit_idx = 2; // need two blank bits at the start
        for (i = 0; i < 3; i++) // for all three bytes
        {
            uint8_t data = buf[i];
            int cbit;
            for (cbit = 0; cbit < 8; cbit++) // for each bit in this byte
            {
                int hi, j;
                hi = ((data & (1 << (7 - cbit))) != 0) ? 6 : 3; // stuff in 3 bits for T0H, or 6 bits for T1H, MSB first
                // each bit is 125 ns (assuming 8 MHz SPI clock)
                // T0H is 400 ns, 3 bits is 375 ns
                // T1H is 800 ns, 6 bits is 750 ns
                for (j = 0; j < hi; j++)
                {
                    int byte_idx = bit_idx / 8;
                    int inner_idx = bit_idx % 8;
                    _buffer[byte_idx] |= 1 << (7 - inner_idx); // MSB first
                    bit_idx++;
                }
                bit_idx += 10 - hi; // TH + TL = 1250 nanoseconds, so 10 total bits
            }
        }
        bit_idx += (30000 / 125) + 1; // 30000 ns blank time
        while ((bit_idx % 8) != 0) {
            bit_idx++; // round up to nearest byte boundary
        }

        len = (bit_idx / 8) + 2; // total number of bytes to send

        #ifdef ROACHRGBLED_DEBUG
        Serial.printf("neopixel buff [%u]: ", len);
        int k;
        for (k = 0; k < len; k++) {
            Serial.printf("%02X", _buffer[k]);
        }
        Serial.printf("\r\n");
        #endif
    }

    // do the SPI transfer
    nrfx_spim_xfer_desc_t xfer_desc =
    {
      .p_tx_buffer = _buffer,
      .tx_length   = len,
      .p_rx_buffer = NULL,
      .rx_length   = 0,
    };
    #ifndef ROACHRGBLED_BLOCKING
    spi_busy = true;
    #endif
    nrfx_spim_xfer(&_spim, &xfer_desc, 0);
    #ifdef ROACHRGBLED_DEBUG
    Serial.printf("SPI xfer returned\r\n");
    #endif
}

void RoachNeoPixel::set(uint8_t r, uint8_t g, uint8_t b, uint8_t brite, bool force)
{
    if (r == _r && g == _g && b == _b && brite == _brite && force == false)
    {
        // don't waste time setting the same colour
        return;
    }

    #ifndef ROACHNEOPIX_BLOCKING
    // if non-blocking mode, then we need to wait until previous sequence has finished
    if (_busy)
    {
        while (!_pwm->EVENTS_SEQEND[_pwmout]) {
            yield();
        }
    }
    _busy = false;
    #endif

    _r = r;
    _g = g;
    _b = b;
    _brite = brite;

    // no real brightness control, except for OFF
    if (brite <= 0)
    {
        r = 0;
        g = 0;
        b = 0;
    }

    uint8_t pixels[] = { g, r, b, }; // define byte order the LED expects
    uint16_t pos = 0; // bit position

    #ifdef ROACHNEOPIX_DEBUG
    Serial.printf("neopixel buff [%u, ", ROACHNEOPIX_BUFFER_SIZE8);
    #endif

    for (uint8_t n = 0; n < 3; n++)
    {
        uint8_t pix = pixels[n];
        for (uint8_t mask = 0x80; mask > 0; mask >>= 1)
        {
            _pattern[pos] = ((pix & mask) != 0 ? 13 : 6) | (0x8000);
            pos++;
        }
    }

    // Zero padding to indicate the end of que sequence
    _pattern[pos++] = 0 | (0x8000); // Seq end
    _pattern[pos++] = 0 | (0x8000); // Seq end

    #ifdef ROACHNEOPIX_DEBUG
    Serial.printf("%u]: ", pos);
    int k;
    for (k = 0; k < pos; k++) {
        Serial.printf("%04X ", _pattern[k]);
    }
    Serial.printf("\r\n");
    #endif

    #ifdef ROACHNEOPIX_ALWAYS_RECONFIG
    pwmConfig();
    #endif

    _pwm->EVENTS_SEQEND[_pwmout]  = 0UL;
    _pwm->TASKS_SEQSTART[_pwmout] = 1UL;

    #ifdef ROACHNEOPIX_BLOCKING
    while (!_pwm->EVENTS_SEQEND[_pwmout]) {
        yield();
    }
    #else
    _busy = true;
    #endif
}

void RoachRgbLed::set(uint32_t x, bool force)
{
    set((uint8_t)((x & 0xFF0000) >> 16), (uint8_t)((x & 0xFF00) >> 8), (uint8_t)((x & 0xFF) >> 0), x ? 0xFF : 0, force);
}

void RoachNeoPixel::set(uint32_t x, bool force)
{
    set((uint8_t)((x & 0xFF0000) >> 16), (uint8_t)((x & 0xFF00) >> 8), (uint8_t)((x & 0xFF) >> 0), x ? 0xFF : 0, force);
}

void RoachRgbLed::setHue(int16_t hue, bool force)
{
    set((uint32_t)RoachHeartbeat_getRgbFromHue(hue), force);
}

void RoachNeoPixel::setHue(int16_t hue, bool force)
{
    set((uint32_t)RoachHeartbeat_getRgbFromHue(hue), force);
}

uint32_t RoachHeartbeat_getRgbFromHue(int16_t h)
{
    uint8_t r, g, b;
    int16_t hue = (h * 1530L + 32768) / 65536;
    if (hue < 510) { // Red to Green-1
        b = 0;
        if (hue < 255) { //   Red to Yellow-1
            r = 255;
            g = hue;       //     g = 0 to 254
        } else {         //   Yellow to Green-1
            r = 510 - hue; //     r = 255 to 1
            g = 255;
        }
    } else if (hue < 1020) { // Green to Blue-1
        r = 0;
        if (hue < 765) { //   Green to Cyan-1
            g = 255;
            b = hue - 510;  //     b = 0 to 254
        } else {          //   Cyan to Blue-1
            g = 1020 - hue; //     g = 255 to 1
            b = 255;
        }
    } else if (hue < 1530) { // Blue to Red-1
        g = 0;
        if (hue < 1275) { //   Blue to Magenta-1
            r = hue - 1020; //     r = 0 to 254
            b = 255;
        } else { //   Magenta to Red-1
            r = 255;
            b = 1530 - hue; //     b = 255 to 1
        }
    } else { // Last 0.5 Red (quicker than % operator)
        r = 255;
        g = b = 0;
    }
    uint32_t ret = r;
    ret <<= 8;
    ret |= g;
    ret <<= 8;
    ret |= b;
    return ret;
}

void RoachRgbLed::spiConfig(void)
{
    nrfx_spim_config_t cfg =
    {
      .sck_pin        = NRFX_SPIM_PIN_NOT_USED,
      .mosi_pin       = NRFX_SPIM_PIN_NOT_USED,
      .miso_pin       = NRFX_SPIM_PIN_NOT_USED,
      .ss_pin         = NRFX_SPIM_PIN_NOT_USED,
      .ss_active_high = false,
      .irq_priority   = 3,
      .orc            = 0xFF,
      .frequency      = NRF_SPIM_FREQ_500K,
      .mode           = NRF_SPIM_MODE_0,
      .bit_order      = NRF_SPIM_BIT_ORDER_MSB_FIRST,
    };
    cfg.sck_pin  = _dotstar ? g_ADigitalPinMap[_pinc] : _pinc;
    cfg.mosi_pin = g_ADigitalPinMap[_pind];
    cfg.frequency = _dotstar ? NRF_SPIM_FREQ_500K : NRF_SPIM_FREQ_8M;
    int ret = nrfx_spim_init(&_spim, &cfg, 
        #if ROACHRGBLED_BLOCKING
            NULL
        #else
            spi_handler
        #endif
        , NULL);

    nrf_gpio_cfg(cfg.sck_pin,
             NRF_GPIO_PIN_DIR_OUTPUT,
             NRF_GPIO_PIN_INPUT_CONNECT,
             NRF_GPIO_PIN_NOPULL,
             NRF_GPIO_PIN_H0H1,
             NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_cfg(cfg.mosi_pin,
             NRF_GPIO_PIN_DIR_OUTPUT,
             NRF_GPIO_PIN_INPUT_DISCONNECT,
             NRF_GPIO_PIN_NOPULL,
             NRF_GPIO_PIN_H0H1,
             NRF_GPIO_PIN_NOSENSE);

    nrf_spim_enable(_spim.p_reg);

    #ifdef ROACHRGBLED_DEBUG
    if (ret == NRFX_SUCCESS) {
        Serial.printf("RoachRgbLed initialized\r\n");
    }
    else {
        Serial.printf("RoachRgbLed initialization error 0x%08X\r\n", ret);
    }
    #endif
}

RoachHeartbeat::RoachHeartbeat(int pin)
{
    _pin = pin;
}

void RoachHeartbeat::begin(void)
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _on = false;
    _last_time = millis();
}

void RoachHeartbeat::play(const uint8_t* ani, bool wait)
{
    if (wait) {
        this->queue(ani);
        return;
    }

    // set NULL to stop animation
    if (ani == NULL) {
        digitalWrite(_pin, LOW);
        _on = false;
    }

    // do not interrupt current animation if the same animation is playing
    // to restart an animation, set it to null first
    if (((uint32_t)ani) == ((uint32_t)_animation)) {
        return;
    }

    // start/restart
    _ani_idx = 0;
    _last_time = millis();
    _animation = (uint8_t*)ani;
    _queued = NULL;
    if (_animation != NULL)
    {
        // definitely start the animation
        // assume first iteration will be ON
        digitalWrite(_pin, HIGH);
        _on = true;
        task();
    }
}

void RoachHeartbeat::queue(const uint8_t* ani)
{
    // do not allow queueing of animation already playing
    if (((uint32_t)ani) == ((uint32_t)_animation)) {
        return;
    }

    _queued = (uint8_t*)ani;
}

void RoachHeartbeat::task(void)
{
    if (_animation == NULL)
    {
        return;
    }

    uint32_t now = millis();
    uint8_t x = _animation[_ani_idx];
    if (_on)
    {
        x >>= 4; // upper 4 nibble indicates time span
        if ((now - _last_time) >= (x * RHB_TIME_MULTI_ON)) // time to turn off
        {
            digitalWrite(_pin, LOW);
            _last_time = now;
            _on = false;
        }
    }
    else
    {
        x &= 0x0F; // lower 4 nibble indicates time span
        if ((now - _last_time) >= (x * RHB_TIME_MULTI_OFF)) // time to turn on
        {
            _last_time = now;
            _ani_idx++;

            x = _animation[_ani_idx];
            if (x == 0) { // animation has finished
                _ani_idx = 0; // loop
                if (_queued != NULL) { // if a queued animation exists, then swap over
                    play((const uint8_t*)_queued);
                    return;
                }
            }

            // do the next ON cycle
            x = _animation[_ani_idx];
            x >>= 4; // upper 4 nibble indicates time span
            if (x > 0) // next ON cycle is valid
            {
                // turn ON
                digitalWrite(_pin, HIGH);
                _on = true;
            }
        }
    }
}
