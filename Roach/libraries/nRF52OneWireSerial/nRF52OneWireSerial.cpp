#include "nRF52OneWireSerial.h"
#include "wiring_private.h"

#ifdef NRFOWS_USE_HW_SERIAL
nRF52OneWireSerial::nRF52OneWireSerial(Uart* ser, NRF_UARTE_Type* uarte, IRQn_Type irqn, int pin)
{
    _serial = ser;
    _nrfUart = uarte;
    _irqn = irqn;
    _pin = pin;
    _pin_hw = g_ADigitalPinMap[pin];
}

void nRF52OneWireSerial::task(void)
{
    int avail = _serial->available();
    if (avail > _last_avail) {
        _last_time = millis();
    }
    _last_avail = avail;
}

void nRF52OneWireSerial::begin(void)
{
    pinMode(_pin, INPUT_PULLUP);
    _nrfUart->PSEL.TXD = 0xFFFFFFFF;
    _nrfUart->PSEL.RXD = _pin_hw;
}

void nRF52OneWireSerial::end(void)
{
    pinMode(_pin, INPUT);
    _serial->end();
    _nrfUart->PSEL.TXD = 0xFFFFFFFF;
    _nrfUart->PSEL.RXD = 0xFFFFFFFF;
}

#else
static nRF52OneWireSerial* active_obj = NULL; // need a static function to be able to call functions from this object

nRF52OneWireSerial::nRF52OneWireSerial(int pin, uint32_t baud, bool invert)
{
    _pin = pin;
    _baud = baud;
    _invert = invert;
}

void nRF52OneWireSerial::begin(void)
{
    active_obj = this; // need a static function to be able to call functions from this object
    
    volatile uint64_t bit_delay, bd;
    bit_delay = (uint64_t)1000000000000;
    bd = _baud * 15692; // calibrated with a logic analyzer
    bit_delay += bd / 2;
    bit_delay /= bd;
    // bit_delay is 553 for 115200 baud

    _tx_delay = bit_delay;
    _rx_delay_centering = (bit_delay / 2) - (2 * NRFX_DELAY_CPU_FREQ_MHZ); // 287 for 115200 is about the highest this goes
    _rx_delay_intrabit  = bit_delay - 35; // 484 to 552 for 115200 baud
    _rx_delay_stopbit   = bit_delay;

    pinMode(_pin, INPUT_PULLUP);

    _pin_mask           = digitalPinToBitMask(_pin);
    NRF_GPIO_Type* port = digitalPinToPort(_pin);
    _pin_input_reg      = portInputRegister(port);
    _pin_output_reg     = portOutputRegister(port);

    listen();
}

void nRF52OneWireSerial::end(void)
{
    detachInterrupt(_pin);
    pinMode(_pin, INPUT);
}

void nRF52OneWireSerial::listen(void)
{
    pinMode(_pin, INPUT_PULLUP);
    _intmask = attachInterrupt(_pin, handle_interrupt, _invert ? RISING : FALLING);
}
#endif

size_t nRF52OneWireSerial::write(uint8_t b)
{
    #ifdef NRFOWS_USE_HW_SERIAL
    //this->checkUartReset();

    pinMode(_pin, OUTPUT);
    _nrfUart->INTENCLR = UARTE_INTENCLR_ENDRX_Msk;
    _nrfUart->PSEL.RXD = 0xFFFFFFFF;
    _nrfUart->PSEL.TXD = _pin_hw;
    #ifdef NRFOWS_USE_TWO_STOP_BITS
    _nrfUart->CONFIG = UARTE_CONFIG_STOP_Two << UARTE_CONFIG_STOP_Pos;
    #endif

    size_t res = _serial->write(b);
    _serial->flush();

    pinMode(_pin, INPUT_PULLUP);
    _nrfUart->PSEL.TXD = 0xFFFFFFFF;
    _nrfUart->PSEL.RXD = _pin_hw;
    _nrfUart->EVENTS_ENDRX = 0x0UL;
    _nrfUart->INTENSET = UARTE_INTENSET_ENDRX_Msk | UARTE_INTENSET_ENDTX_Msk;
    #ifdef NRFOWS_USE_TWO_STOP_BITS
    _nrfUart->CONFIG = UARTE_CONFIG_STOP_One << UARTE_CONFIG_STOP_Pos;
    #endif

    return res;
    #else
    __disable_irq();

    detachInterrupt(_pin);
    digitalWrite(_pin, _invert ? LOW : HIGH);
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, _invert ? LOW : HIGH);

    // By declaring these as local variables, the compiler will put them
    // in registers _before_ disabling interrupts and entering the
    // critical timing sections below, which makes it a lot easier to
    // verify the cycle timings
    volatile uint32_t* reg = _pin_output_reg;
    uint32_t reg_mask = _pin_mask;
    uint32_t inv_mask = ~_pin_mask;
    bool inv = _invert;
    uint32_t delay = _tx_delay;

    if (inv) {
        b = ~b;
    }

    // turn off interrupts for a clean txmit
    NRF_GPIOTE->INTENCLR = _intmask;

    // Write the start bit
    if (inv) {
        *reg |= reg_mask;
    }
    else {
        *reg &= inv_mask;
    }

    nrfows_delayloops(delay);

    // Write each of the 8 bits
    for (uint8_t i = 8; i > 0; --i)
    {
        if ((b & 1) != 0) { // choose bit
            *reg |= reg_mask; // send 1
        }
        else {
            *reg &= inv_mask; // send 0
        }

        nrfows_delayloops(delay);
        b >>= 1;
    }

    // stop bit, restore pin to natural state
    if (inv) {
        *reg &= inv_mask;
    }
    else {
        *reg |= reg_mask;
    }

    NRF_GPIOTE->INTENSET = _intmask;

    //nrfows_delayloops(delay / 2);
    #ifdef NRFOWS_USE_TWO_STOP_BITS
    nrfows_delayloops(delay * 2);
    #else
    nrfows_delayloops(delay);
    #endif

    __enable_irq();

    listen();

    return 1;
    #endif
}

size_t nRF52OneWireSerial::write(const uint8_t *buffer, size_t size)
{
    #ifdef NRFOWS_USE_HW_SERIAL
    //this->checkUartReset();

    pinMode(_pin, OUTPUT);
    _nrfUart->INTENCLR = UARTE_INTENCLR_ENDRX_Msk;
    _nrfUart->PSEL.RXD = 0xFFFFFFFF;
    _nrfUart->PSEL.TXD = _pin_hw;
    #ifdef NRFOWS_USE_TWO_STOP_BITS
    _nrfUart->CONFIG = UARTE_CONFIG_STOP_Two << UARTE_CONFIG_STOP_Pos;
    #endif

    size_t res = _serial->write(buffer, size);
    _serial->flush();

    pinMode(_pin, INPUT_PULLUP);
    _nrfUart->PSEL.TXD = 0xFFFFFFFF;
    _nrfUart->PSEL.RXD = _pin_hw;
    _nrfUart->EVENTS_ENDRX = 0x0UL;
    _nrfUart->INTENSET = UARTE_INTENSET_ENDRX_Msk | UARTE_INTENSET_ENDTX_Msk;
    #ifdef NRFOWS_USE_TWO_STOP_BITS
    _nrfUart->CONFIG = UARTE_CONFIG_STOP_One << UARTE_CONFIG_STOP_Pos;
    #endif

    return res;
    #else
    detachInterrupt(_pin);
    digitalWrite(_pin, _invert ? LOW : HIGH);
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, _invert ? LOW : HIGH);

    // By declaring these as local variables, the compiler will put them
    // in registers _before_ disabling interrupts and entering the
    // critical timing sections below, which makes it a lot easier to
    // verify the cycle timings
    volatile uint32_t* reg = _pin_output_reg;
    uint32_t reg_mask = _pin_mask;
    uint32_t inv_mask = ~_pin_mask;
    bool inv = _invert;
    uint32_t delay = _tx_delay;

    __disable_irq();

    // turn off interrupts for a clean txmit
    NRF_GPIOTE->INTENCLR = _intmask;

    int j;
    for (j = 0; size > 0; j++)
    {
        uint8_t b = (inv == false) ? buffer[j] : ~buffer[j];

        // Write the start bit
        if (inv) {
            *reg |= reg_mask;
        }
        else {
            *reg &= inv_mask;
        }

        nrfows_delayloops(delay);

        // Write each of the 8 bits
        for (uint8_t i = 8; i > 0; --i)
        {
            if ((b & 1) != 0) { // choose bit
                *reg |= reg_mask; // send 1
            }
            else {
                *reg &= inv_mask; // send 0
            }

            nrfows_delayloops(delay);
            b >>= 1;
        }

        // stop bit, restore pin to natural state
        if (inv) {
            *reg &= inv_mask;
        }
        else {
            *reg |= reg_mask;
        }

        size--;
        //nrfows_delayloops(delay / (size > 0 ? 2 : 1));
        #ifdef NRFOWS_USE_TWO_STOP_BITS
        nrfows_delayloops(delay * 2);
        #else
        nrfows_delayloops(delay);
        #endif
    }

    NRF_GPIOTE->INTENSET = _intmask;

    __enable_irq();

    listen();

    return (size_t)j;
    #endif
}

#ifdef NRFOWS_USE_HW_SERIAL
void nRF52OneWireSerial::checkUartReset(void)
{
    uint32_t now = millis();
    if (_last_tx_time != 0 && _last_txrst_time != 0)
    {
        if ((now - _last_tx_time) >= 50 && (now - _last_txrst_time) >= 1000) {
            uint32_t br = _nrfUart->BAUDRATE;
            uint32_t tptr = _nrfUart->TXD.PTR;
            uint32_t rptr = _nrfUart->RXD.PTR;
            //_serial->end();
            //_serial->begin(19200);
            NVIC_DisableIRQ(_irqn);
            _nrfUart->INTENCLR = UARTE_INTENSET_ENDRX_Msk | UARTE_INTENSET_ENDTX_Msk;
            _nrfUart->EVENTS_RXTO = 0;
            _nrfUart->EVENTS_TXSTOPPED = 0;
            _nrfUart->TASKS_STOPRX = 0x1UL;
            _nrfUart->TASKS_STOPTX = 0x1UL;
            _nrfUart->TXD.PTR = 0;
            _nrfUart->RXD.PTR = 0;
            // Wait for TXSTOPPED event and for RXTO event
            // This is required before disabling UART to fully power down transceiver PHY.
            // Otherwise transceiver will continue to consume ~900uA
            while ( !(_nrfUart->EVENTS_TXSTOPPED && _nrfUart->EVENTS_RXTO) ) yield();
            _nrfUart->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
            _nrfUart->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
            _nrfUart->TXD.PTR = tptr;
            _nrfUart->EVENTS_ENDTX = 0x0UL;
            _nrfUart->RXD.PTR = rptr;
            _nrfUart->RXD.MAXCNT = 1;
            _nrfUart->TASKS_STARTRX = 0x1UL;
            _nrfUart->INTENSET = UARTE_INTENSET_ENDRX_Msk | UARTE_INTENSET_ENDTX_Msk;
            NVIC_ClearPendingIRQ(_irqn);
            NVIC_SetPriority(_irqn, 3);
            NVIC_EnableIRQ(_irqn);
            _nrfUart->BAUDRATE = br;
            _last_txrst_time = now;
        }
    }
    else
    {
        _last_txrst_time = now;
    }
    _last_tx_time = now;
}
#endif

#ifndef NRFOWS_USE_HW_SERIAL

#define NRFOWS_PIN_READ() ((*reg) & reg_mask)

void nRF52OneWireSerial::recv(void)
{
    uint8_t d = 0;
    // By declaring these as local variables, the compiler will put them
    // in registers _before_ disabling interrupts and entering the
    // critical timing sections below, which makes it a lot easier to
    // verify the cycle timings
    volatile uint32_t* reg = _pin_input_reg;
    uint32_t reg_mask = _pin_mask;
    bool inv = _invert;

    if (inv ? NRFOWS_PIN_READ() : !NRFOWS_PIN_READ())
    {
        __disable_irq();

        uint32_t dly_ctr = _rx_delay_centering;
        uint32_t dly_intra = _rx_delay_intrabit;

        NRF_GPIOTE->INTENCLR = _intmask;
        // Wait approximately 1/2 of a bit width to "center" the sample
        nrfows_delayloops(dly_ctr);

        // Read each of the 8 bits
        for (uint8_t i = 8; i > 0; --i)
        {
            nrfows_delayloops(dly_intra);
            d >>= 1;
            if (NRFOWS_PIN_READ()) {
                d |= 0x80;
            }
        }
        if (inv){
            d = ~d;
        }

        uint32_t n = (_rx_fifo_w + 1) % NRFOWS_RX_BUFF_SIZE;
        if (n != _rx_fifo_r) {
            _rx_fifo[_rx_fifo_w] = d;
            _rx_fifo_w = n;
            _rx_cnt += 1;
        }

        // skip the stop bit
        nrfows_delayloops(_rx_delay_stopbit);

        NRF_GPIOTE->INTENSET = _intmask;

        _last_time = millis();
    }

    __enable_irq();
}

inline void nRF52OneWireSerial::handle_interrupt()
{
    active_obj->recv();
}

#endif

int nRF52OneWireSerial::available(void)
{
    #ifdef NRFOWS_USE_HW_SERIAL
    return _serial->available();
    #else
    return _rx_cnt;
    #endif
}

int nRF52OneWireSerial::read(void)
{
    #ifdef NRFOWS_USE_HW_SERIAL
    return _serial->read();
    #else
    if (_rx_cnt == 0) {
        return -1;
    }
    __disable_irq();
    _rx_cnt -= 1;
    uint8_t x = _rx_fifo[_rx_fifo_r];
    _rx_fifo_r += 1;
    _rx_fifo_r %= NRFOWS_RX_BUFF_SIZE;
    __enable_irq();
    return (int)x;
    #endif
}

int nRF52OneWireSerial::peek(void)
{
    #ifdef NRFOWS_USE_HW_SERIAL
    return _serial->peek();
    #else
    if (_rx_cnt == 0) {
        return -1;
    }
    __disable_irq();
    uint8_t x = _rx_fifo[_rx_fifo_r];
    __enable_irq();
    return (int)x;
    #endif
}

void nRF52OneWireSerial::flush(void)
{
    #ifdef NRFOWS_USE_HW_SERIAL
    _serial->flush();
    #else
    //__disable_irq();
    //_rx_cnt = 0;
    //_rx_fifo_r = 0;
    //_rx_fifo_w = 0;
    //__enable_irq();
    #endif
}

uint32_t nRF52OneWireSerial::echo(Stream* dest, bool flush, Stream* dbg)
{
    uint32_t ret = 0;
    int i = dest->available();
    if (i > 0)
    {
        bool used_buffer = false;
        ret += i;
        if (i >= 8 && i < NRFOWS_TX_BUFF_SIZE)
        {
            used_buffer = true;
            dest->readBytes(_tx_buff, (size_t)i);
            this->write((const uint8_t*) _tx_buff, (size_t)i);
            dest->write((const uint8_t*) _tx_buff, (size_t)i);
        }
        else
        {
            int j;
            for (j = 0; j < i; j++)
            {
                uint8_t x = (uint8_t)dest->read();
                this->write(x);
                dest->write(x);
            }
        }
        if (flush || used_buffer)
        {
            dest->flush();
        }
    }

    i = this->available();
    if (i > 0)
    {
        bool used_buffer = false;
        ret += i;
        if (i >= 8 && i < NRFOWS_TX_BUFF_SIZE)
        {
            used_buffer = true;
            this->readBytes(_tx_buff, (size_t)i);
            dest->write((const uint8_t*) _tx_buff, (size_t)i);
            if (dbg) {
                dbg->write((const uint8_t*) _tx_buff, (size_t)i);
            }
        }
        else
        {
            int j;
            for (j = 0; j < i; j++)
            {
                uint8_t x = (uint8_t)this->read();
                dest->write(x);
                if (dbg) {
                    dbg->write(x);
                }
            }
        }
        if (flush || used_buffer)
        {
            dest->flush();
        }
    }
    return ret;
}