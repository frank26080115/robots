#ifndef _ROACHBUTTON_H_
#define _ROACHBUTTON_H_

#include <Arduino.h>

class RoachButton
{
    public:
        RoachButton(int pin, bool intr_mode = true, int rep = 500, int db = 50);
        void begin(void);
        void task(void);
        void poll(void);
        bool hasPressed(bool clr);
        uint32_t isHeld(void);
        void disableUntilRelease(void);
        void fakePress(void);
        void fakeToggle(void);
        void handleIsr(void);
        inline int getPin(void) { return _pin; };
    private:
        int _pin;
        bool _intr_mode;
        int _pin_idx;
        volatile uint32_t _last_down_time, _last_up_time, _last_change_time;
        unsigned int _rep, _db;
        volatile bool _pressed, _last_poll;
        bool _disabled = false;
        bool _faketoggle = false;
        int _init_high_cnt;
        bool _init_done;
};

void RoachButton_allBegin(void);
void RoachButton_allTask(void);
void RoachButton_clearAll(void);
uint32_t RoachButton_hasAnyPressed(void);
uint32_t RoachButton_isAnyHeld(void);

#endif
