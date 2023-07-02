#ifndef _ROACHHEARTBEAT_H_
#define _ROACHHEARTBEAT_H_

#include <Arduino.h>
#include "nrfx_spim.h"

#define RHB_TIME_MULTI_ON   50
#define RHB_TIME_MULTI_OFF 125

#define RHB_HW_PIN_DOTSTAR_CLK 6
#define RHB_HW_PIN_DOTSTAR_DAT 8
#define RHB_HW_PIN_NEOPIXEL    8
#define RHB_HW_PIN_NEOPIXEL_UNUSED     (1 | (1 << 5))

#define RHB_HW_PIN_ITSYBITSY_LED 3
#define RHB_HW_PIN_NRFEXPR_LED_R 3
#define RHB_HW_PIN_NRFEXPR_LED_B 4

//#define ROACHRGBLED_DEBUG
//#define ROACHRGBLED_BLOCKING

class RoachHeartbeat
{
    public:
        RoachHeartbeat(int pin);
        void begin(void);
        void play(const uint8_t*, bool wait = false);
        void queue(const uint8_t*);
        void task(void);

    private:
        int _pin;
        uint32_t _last_time;
        bool _on;
        uint8_t* _animation = NULL;
        uint8_t* _queued = NULL;
        int _ani_idx;
};

class RoachRgbLed
{
    public:
        RoachRgbLed(bool dotstar = true, int pind = RHB_HW_PIN_DOTSTAR_DAT, int pinc = RHB_HW_PIN_DOTSTAR_CLK, NRF_SPIM_Type* p_spi = NRF_SPIM2);
        void begin(void);
        void set(uint8_t r, uint8_t g, uint8_t b, uint8_t brite = 0xFF, bool force = false);
        void spiConfig();
    private:
        bool _dotstar;
        int _pinc, _pind;
        NRF_SPIM_Type* _spi;
        nrfx_spim_t _spim;
        uint8_t _r, _g, _b, _brite;
        #define ROACHRGBLED_BUFFER_SIZE (32 * 3)
        uint8_t _buffer[ROACHRGBLED_BUFFER_SIZE];
};

void RoachWdt_init(uint32_t tmr_ms);
void RoachWdt_feed(void);

#endif
