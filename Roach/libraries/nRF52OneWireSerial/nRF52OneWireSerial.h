#ifndef _NRF52ONEWIRESERIAL_H_
#define _NRF52ONEWIRESERIAL_H_

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif
extern void nrfows_delayloops(uint32_t loops);
#ifdef __cplusplus
}
#endif

#define NRFOWS_USE_HW_SERIAL
#define NRFOWS_RX_BUFF_SIZE 512

class nRF52OneWireSerial : public Stream
{
    public:
        #ifdef NRFOWS_USE_HW_SERIAL
        nRF52OneWireSerial(Uart* ser, NRF_UARTE_Type* uarte, int pin);
        void task(void);
        #else
        nRF52OneWireSerial(int pin, uint32_t baud, bool invert);
        #endif
        void begin(void);
        void end(void);
        inline uint32_t lastTime(void) { return _last_time; };

        using Print::write;

        virtual size_t write(uint8_t byte);
        virtual size_t write(const uint8_t *buffer, size_t size);
        virtual int available(void);
        virtual int read(void);
        virtual int peek(void);
        virtual void flush(void);
        operator bool() { return true; }

        #ifndef NRFOWS_USE_HW_SERIAL
        static inline void handle_interrupt() __attribute__((__always_inline__));
        void recv(void);

        uint32_t _tx_delay, _rx_delay_centering, _rx_delay_intrabit, _rx_delay_stopbit;
        #endif

        uint32_t echo(Stream* dest, bool flush = false, Stream* dbg = NULL);
        inline void echoForever(Stream* dest) { while (true) { yield(); echo(dest); } };

    private:
        int _pin;
        volatile uint32_t _last_time = 0;
        #ifndef NRFOWS_USE_HW_SERIAL
        uint32_t _baud;
        bool _invert;
        uint8_t _rx_fifo[NRFOWS_RX_BUFF_SIZE];
        volatile uint16_t _rx_fifo_r = 0, _rx_fifo_w = 0;
        volatile int _rx_cnt = 0;
        uint32_t _pin_mask;
        volatile uint32_t* _pin_input_reg;
        volatile uint32_t* _pin_output_reg;
        volatile uint32_t _intmask;
        void listen(void);
        #else
        Uart* _serial;
        uint32_t _pin_hw;
        uint32_t _last_avail = 0;
        NRF_UARTE_Type* _nrfUart;
        #endif
};

#endif
