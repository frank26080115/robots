#ifndef _ROACHHEARTBEAT_H_
#define _ROACHHEARTBEAT_H_

#include <Arduino.h>
#include "nrfx_spim.h"

#define RHB_TIME_MULTI_ON   50
#define RHB_TIME_MULTI_OFF 125

// hardware pins are defined for Adafruit nRF52840 products

#define RHB_HW_PIN_DOTSTAR_CLK 6
#define RHB_HW_PIN_DOTSTAR_DAT 8
#define RHB_HW_PIN_NEOPIXEL    8
#define RHB_HW_PIN_NEOPIXEL_UNUSED     (1 | (1 << 5))

#define RHB_HW_PIN_ITSYBITSY_LED 3
#define RHB_HW_PIN_NRFEXPR_LED_R 3
#define RHB_HW_PIN_NRFEXPR_LED_B 4

//#define ROACHRGBLED_DEBUG
//#define ROACHNEOPIX_DEBUG
//#define ROACHRGBLED_BLOCKING
//#define ROACHNEOPIX_BLOCKING

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
        void set(uint32_t x, bool force = false);
        void setHue(int16_t hue, bool force = false);
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

class RoachNeoPixel
{
    public:
        RoachNeoPixel(int pind = RHB_HW_PIN_NEOPIXEL, NRF_PWM_Type* p_pwm = NULL, int pwm_out = -1);
        void begin(void);
        void set(uint8_t r, uint8_t g, uint8_t b, uint8_t brite = 0xFF, bool force = false);
        void set(uint32_t x, bool force = false);
        void setHue(int16_t hue, bool force = false);
    private:
        int _pind, _pwmout;
        NRF_PWM_Type* _pwm;
        #ifndef ROACHNEOPIX_BLOCKING
        bool _busy = false;
        #endif
        uint8_t _r, _g, _b, _brite;
        #define ROACHNEOPIX_BUFFER_SIZE8 (3 * 8 * sizeof(uint16_t) + 2 * sizeof(uint16_t))
        uint16_t _pattern[ROACHNEOPIX_BUFFER_SIZE8 / 2];
};

uint32_t RoachHeartbeat_getRgbFromHue(int16_t h);

void RoachWdt_init(uint32_t tmr_ms);
void RoachWdt_feed(void);

#endif
