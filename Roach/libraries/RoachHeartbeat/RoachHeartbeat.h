#ifndef _ROACHHEARTBEAT_H_
#define _ROACHHEARTBEAT_H_

#include <Arduino.h>

#define RHB_TIME_MULTI_ON   50
#define RHB_TIME_MULTI_OFF 125

#define RHB_HW_PIN_DOTSTAR_CLK 6
#define RHB_HW_PIN_DOTSTAR_DAT 8
#define RHB_HW_PIN_NEOPIXEL    8

class RoachHeartbeat
{
    public:
        RoachHeartbeat(int pin);
        void begin(void);
        void play(const uint8_t*);
        void task(void);

    private:
        int _pin;
        uint32_t _last_time;
        bool _on;
        uint8_t* _animation = NULL;;
        int _ani_idx;
};

class RoachRgbLed
{
    public:
        RoachRgbLed(NRF_SPIM_Type* p_spi, bool dotstar, int pind = RHB_HW_PIN_DOTSTAR_DAT, int pinc = RHB_HW_PIN_DOTSTAR_CLK); // use NRF_SPIM2
        void begin(void);
        void set(uint8_t r, uint8_t g, uint8_t b, uint8_t brite = 0xFF, bool force = false, bool wait = true);
        void spiWait(bool);
        void spiConfig();
        void spiCache(void);
        void spiRestore(void);
    private:
        bool _dotstar;
        int _pinc, _pind;
        NRF_SPIM_Type* _spi;
        uint8_t _r, _g, _b, _brite;
        #define ROACHRGBLED_BUFFER_SIZE 64
        uint8_t _buffer[ROACHRGBLED_BUFFER_SIZE];

        uint32_t _spiCache_freq, _spiCache_config, _spiCache_pinMosi, _spiCache_pinMiso, _spiCache_pinSck, _spiCache_rxdPtr, _spiCache_rxdMaxcnt, _spiCache_txdPtr, _spiCache_txdMaxcnt;
};

void RoachWdt_init(uint32_t tmr_ms);
void RoachWdt_feed(void);

#endif
