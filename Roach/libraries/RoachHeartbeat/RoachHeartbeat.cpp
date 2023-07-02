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

void RoachRgbLed::begin(void)
{
    spiConfig();
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

    int len;

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
    if (_animation == NULL) {
        digitalWrite(_pin, LOW);
        _on = false;
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
    uint32_t t;
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
