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

RoachRgbLed::RoachRgbLed(NRF_SPIM_Type* p_spi, bool dotstar, int pind, int pinc)
{
    _spi = p_spi;
    _dotstar = dotstar;
    _pind = pind;
    _pinc = pinc;
}

void RoachRgbLed::begin(void)
{
    spiConfig();
    set(0, 0, 0, 0, true, false);
}

void RoachRgbLed::set(uint8_t r, uint8_t g, uint8_t b, uint8_t brite, bool force, bool wait)
{
    if (r == _r && g == _g && b == _b && brite == _brite && force == false)
    {
        // don't waste time setting the same colour
        return;
    }

    if (wait)
    {
        // wait for previous transaction to complete
        spiWait(true);
    }
    _spi->EVENTS_ENDTX = 0;

    _r = r;
    _g = g;
    _b = b;
    _brite = brite;

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
        // send out these bytes over SPI
        _spi->TXD.MAXCNT = 12;
        _spi->TXD.PTR = (uint32_t)_buffer;
        _spi->TASKS_START = 1UL;
    }
    else
    {
        memset(_buffer, 0, ROACHRGBLED_BUFFER_SIZE);
        uint8_t buf[3] = {g, r, b}; // define the byte order expected
        int i, bit_idx = 0;
        for (i = 0; i < 3; i++) // for all three bytes
        {
            uint8_t data = buf[i];
            int cbit;
            for (cbit = 0; cbit < 8; cbit++) // for each bit in this byte
            {
                int hi, j;
                hi = ((b & (1 << (7 - cbit))) != 0) ? 6 : 3; // stuff in 3 bits for T0H, or 6 bits for T1H, MSB first
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
        int len = (bit_idx / 8) + 2; // total number of bytes to send
        // send out SPI
        _spi->TXD.MAXCNT = len;
        _spi->TXD.PTR = (uint32_t)_buffer;
        _spi->TASKS_START = 1UL;
    }
}

void RoachRgbLed::spiWait(bool clr)
{
    // wait for previous transaction to complete
    while (_spi->EVENTS_ENDTX == 0) {
        yield();
    }
    if (clr) {
        _spi->EVENTS_ENDTX = 0;
    }
}

void RoachRgbLed::spiConfig(void)
{
    spiCache();
    if (_dotstar)
    {
        pinMode(_pinc, OUTPUT);
        digitalWrite(_pinc, HIGH);
        pinMode(_pind, OUTPUT);
        _spi->PSELSCK   = g_ADigitalPinMap[_pinc];
        _spi->PSELMOSI  = g_ADigitalPinMap[_pind];
        _spi->FREQUENCY = SPI_FREQUENCY_FREQUENCY_K500; // max clock acceptable is 512 kHz
        _spi->CONFIG    = 0; // MSB first, mode 0
        _spi->ENABLE    = 7;
    }
    else
    {
        pinMode(_pind, OUTPUT);
        digitalWrite(_pind, LOW);
        _spi->PSELSCK    = 0xFFFFFFFF;
        _spi->PSELMOSI   = g_ADigitalPinMap[_pind];
        _spi->FREQUENCY  = SPI_FREQUENCY_FREQUENCY_M8; // each bit is 125 ns
        _spi->CONFIG     = 0; // MSB first, mode 0
        _spi->ENABLE     = 7;
    }
}

void RoachRgbLed::spiCache(void)
{
    _spiCache_freq      = _spi->FREQUENCY ;
    _spiCache_config    = _spi->CONFIG    ;
    _spiCache_pinMosi   = _spi->PSELMOSI  ;
    _spiCache_pinMiso   = _spi->PSELMISO  ;
    _spiCache_pinSck    = _spi->PSELSCK   ;
    _spiCache_txdPtr    = _spi->TXD.PTR   ;
    _spiCache_txdMaxcnt = _spi->TXD.MAXCNT;
    _spiCache_rxdPtr    = _spi->RXD.PTR   ;
    _spiCache_rxdMaxcnt = _spi->RXD.MAXCNT;
}

void RoachRgbLed::spiRestore(void)
{
    _spi->FREQUENCY  = _spiCache_freq     ;
    _spi->CONFIG     = _spiCache_config   ;
    _spi->PSELMOSI   = _spiCache_pinMosi  ;
    _spi->PSELMISO   = _spiCache_pinMiso  ;
    _spi->PSELSCK    = _spiCache_pinSck   ;
    _spi->TXD.PTR    = _spiCache_txdPtr   ;
    _spi->TXD.MAXCNT = _spiCache_txdMaxcnt;
    _spi->RXD.PTR    = _spiCache_rxdPtr   ;
    _spi->RXD.MAXCNT = _spiCache_rxdMaxcnt;
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

void RoachHeartbeat::play(const uint8_t* ani)
{
    if (_animation == NULL) {
        digitalWrite(_pin, LOW);
        _on = false;
    }
    _ani_idx = 0;
    _last_time = millis();
    _animation = (uint8_t*)ani;
    if (_animation != NULL)
    {
        digitalWrite(_pin, HIGH);
        _on = true;
        task();
    }
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
        x >>= 4;
        if ((now - _last_time) >= (x * RHB_TIME_MULTI_ON))
        {
            digitalWrite(_pin, LOW);
            _last_time = now;
            _on = false;
        }
    }
    else
    {
        x &= 0x0F;
        if ((now - _last_time) >= (x * RHB_TIME_MULTI_OFF))
        {
            _last_time = now;
            _ani_idx++;

            x = _animation[_ani_idx];
            if (x == 0) {
                _ani_idx = 0;
            }
            x = _animation[_ani_idx];
            x >>= 4;
            if (x > 0)
            {
                digitalWrite(_pin, HIGH);
                _on = true;
            }
        }
    }
}
