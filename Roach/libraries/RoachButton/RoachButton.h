#ifndef _ROACHBUTTON_H_
#define _ROACHBUTTON_H_

#include <Arduino.h>

class RoachButton
{
    public:
        RoachButton(int pin, int rep = 500, int db = 50);
        void begin(void);
        bool hasPressed(bool clr);
        uint32_t isHeld(void);
        void disableUntilRelease(void);
        void fakePress(void);
        void fakeToggle(void);
        void handleIsr(void);
    private:
        int _pin;
        volatile uint32_t _last_down_time, _last_up_time, _last_change_time;
        int _rep, _db;
        volatile bool _pressed;
        bool _disabled = false;
        bool _faketoggle = false;
};

void RoachButton_allBegin(void);
void RoachButton_clearAll(void);
bool RoachButton_hasAnyPressed(void);
bool RoachButton_isAnyHeld(void);

#endif
