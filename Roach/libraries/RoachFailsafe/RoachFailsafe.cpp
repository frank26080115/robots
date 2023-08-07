#include "RoachFailsafe.h"

static uint32_t rfailsafe_timeout = 1000;
static uint32_t rfailsafe_lastTime = 0;
static bool is_safe = false;
static void (*cb_onSafe)(void) = NULL;
static void (*cb_onFail)(void) = NULL;

void rfailsafe_init(uint32_t timeout)
{
    rfailsafe_timeout = timeout;
    rfailsafe_lastTime = 0;
}

void rfailsafe_task(uint32_t now)
{
    if ((now - rfailsafe_lastTime) >= rfailsafe_timeout)
    {
        if (is_safe)
        {
            is_safe = false;
            if (rfailsafe_cb_onFail != NULL) {
                rfailsafe_cb_onFail();
            }
        }
    }
    else
    {
        if (is_safe == false)
        {
            is_safe = true;
            if (rfailsafe_cb_onSafe != NULL) {
                rfailsafe_onSafe();
            }
        }
    }
}

void rfailsafe_feed(uint32_t now)
{
    rfailsafe_lastTime = now;
    rfailsafe_task(now);
}

bool rfailsafe_isSafe(void)
{
    return is_safe;
}

void rfailsafe_attachOnFail(void(*cb)(void))
{
    rfailsafe_cb_onFail = cb;
}

void rfailsafe_attachOnSafe(void(*cb)(void))
{
    rfailsafe_cb_onSafe = cb;
}
