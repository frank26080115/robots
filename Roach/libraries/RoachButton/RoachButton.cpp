#include "RoachButton.h"

#define ROACHBTN_CNT_MAX 12

static int roachbtn_cnt = 0;
static RoachButton* roachbtn_insttbl[ROACHBTN_CNT_MAX];

static void _pin_irq(uint8_t id);

#define _pin_irqn(n) \
  void _pin_irqn##_##n(void) { \
    _pin_irq(n);\
  }

extern "C"
{
  _pin_irqn(0)
  _pin_irqn(1)
  _pin_irqn(2)
  _pin_irqn(3)
  _pin_irqn(4)
  _pin_irqn(5)
  _pin_irqn(6)
  _pin_irqn(7)
  _pin_irqn(8)
  _pin_irqn(9)
  _pin_irqn(10)
  _pin_irqn(11)
  // add more to match ROACHBTN_CNT_MAX
}

static voidFuncPtr _pin_irq_ptr[ROACHBTN_CNT_MAX] =
{
    _pin_irqn_0, _pin_irqn_1, _pin_irqn_2, _pin_irqn_3,
    _pin_irqn_4, _pin_irqn_5, _pin_irqn_6, _pin_irqn_7,
    _pin_irqn_8, _pin_irqn_9, _pin_irqn_10, _pin_irqn_11
};

RoachButton::RoachButton(int pin, int rep, int db)
{
    _pin = pin;
    _rep = rep;
    _db = db;
    roachbtn_insttbl[roachbtn_cnt] = this;
    roachbtn_cnt += 1;
}

void RoachButton::begin(void)
{
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(_pin, _pin_irq_ptr[roachbtn_cnt], CHANGE);
    _pressed = false;
    _last_down_time = 0;
    _last_up_time = 0;
    _last_change_time = 0;
}

static void _pin_irq(uint8_t id)
{
    roachbtn_insttbl[id]->handleIsr();
}

void RoachButton::handleIsr(void)
{
    uint32_t now = millis();
    _last_change_time = now;
    if (digitalRead(_pin) == LOW)
    {
        _last_down_time = now;
        if ((now - _last_up_time) >= _db && (now - _last_down_time) >= _db)
        {
            _pressed = true;
        }
    }
    else
    {
        _last_up_time = now;
    }
}

bool RoachButton::hasPressed(bool clr)
{
    __disable_irq();
    uint32_t now = millis();
    if (_disabled)
    {
        if (digitalRead(_pin) != LOW)
        {
            _disabled = false;
        }
        else
        {
            _pressed = false;
            __enable_irq();
            return false;
        }
    }
    bool x = _pressed;
    if (_rep != 0 && x == false && digitalRead(_pin) == LOW)
    {
        if ((now - _last_down_time) >= _rep) {
            x = true;
            _last_down_time = now;
        }
    }
    if (clr) {
        _pressed = false;
    }
    __enable_irq();
    return x;
}

uint32_t RoachButton::isHeld(void)
{
    if (_disabled)
    {
        if (digitalRead(_pin) != LOW)
        {
            _disabled = false;
        }
        else
        {
            return 0;
        }
    }

    uint32_t d = 0;
    if (digitalRead(_pin) != LOW)
    {
        return 0;
    }
    uint32_t now = millis();
    __disable_irq();
    d = now - _last_down_time;
    d = d <= 0 ? 1 : d;
    __enable_irq();
    return d;
}

void RoachButton::disableUntilRelease(void)
{
    _disabled = true;
}

void RoachButton::fakePress(void)
{
    _pressed = true;
}

void RoachButton_clearAll(void)
{
    int i;
    for (i = 0; i < roachbtn_cnt; i++) {
        roachbtn_insttbl[i]->hasPressed(true);
    }
}

void RoachButton_allBegin(void)
{
    int i;
    for (i = 0; i < roachbtn_cnt; i++) {
        roachbtn_insttbl[i]->begin();
    }
}

bool RoachButton_hasAnyPressed(void)
{
    bool x = false;
    int i;
    for (i = 0; i < roachbtn_cnt; i++) {
        x |= roachbtn_insttbl[i]->hasPressed(false);
    }
    return x;
}

bool RoachButton_isAnyHeld(void)
{
    bool x = false;
    int i;
    for (i = 0; i < roachbtn_cnt; i++) {
        x |= roachbtn_insttbl[i]->isHeld() > 0;
    }
    return x;
}
