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

#define NRFOWS_RX_BUFF_SIZE 512

class nRF52OneWireSerial : public Stream
{
    public:
        nRF52OneWireSerial(int pin, uint32_t baud, bool invert);
        void begin(void);

        using Print::write;

        virtual size_t write(uint8_t byte);
        virtual size_t write(const uint8_t *buffer, size_t size);
        virtual int available(void);
        virtual int read(void);
        virtual int peek(void);
        virtual void flush(void);
        operator bool() { return true; }

        static inline void handle_interrupt() __attribute__((__always_inline__));
        void recv(void);

        uint32_t _tx_delay, _rx_delay_centering, _rx_delay_intrabit, _rx_delay_stopbit;

    private:
        int _pin;
        uint32_t _baud;
        bool _invert;
        uint8_t _rx_fifo[NRFOWS_RX_BUFF_SIZE];
        volatile uint16_t _rx_fifo_r = 0, _rx_fifo_w = 0;
        volatile int _rx_cnt = 0;
        volatile uint32_t _last_time = 0;
        uint32_t _pin_mask;
        volatile uint32_t* _pin_input_reg;
        volatile uint32_t* _pin_output_reg;
        volatile uint32_t _intmask;
        void listen(void);
};

#endif
