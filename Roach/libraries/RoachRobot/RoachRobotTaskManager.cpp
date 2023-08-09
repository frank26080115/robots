#include "RoachRobotTaskManager.h"

static uint32_t task_interval;
static uint32_t failsafe_timeout;
static uint32_t last_time = 0;
static uint32_t last_rx = 0;
static uint32_t fail_time = 0;
static bool need_full_init = true;

enum
{
    RTMGR_STATE_INIT,
    RTMGR_STATE_START,
    RTMGR_STATE_RUN,
    RTMGR_STATE_PREFAIL,
    RTMGR_STATE_POSTFAIL,
    RTMGR_STATE_SIM,
    RTMGR_STATE_ENDPRE,
    RTMGR_STATE_ENDPOST,
};

static uint8_t statemachine = RTMGR_STATE_INIT;

void rtmgr_init(uint32_t intv, uint32_t timeout)
{
    task_interval = intv;
    failsafe_timeout = timeout;
    statemachine = RTMGR_STATE_START;
}

void rtmgr_task(uint32_t now)
{
    bool has_cmd;
    if (statemachine == RTMGR_STATE_RUN || statemachine == RTMGR_STATE_SIM)
    {
        if ((has_cmd = radio.available()) || (now - last_time) >= (task_interval + (task_interval / 2)))
        {
            last_time = now;
            last_rx = (has_cmd || statemachine == RTMGR_STATE_SIM) ? now : last_rx;
            rtmgr_taskPeriodic(has_cmd);
        }
    }

    switch (statemachine)
    {
        case RTMGR_STATE_RUN:
        case RTMGR_STATE_SIM:
            if (failsafe_timeout == 0)
            {
                if (radio.isConnected() == false && statemachine != RTMGR_STATE_SIM)
                {
                    fail_time = now;
                    statemachine = RTMGR_STATE_PREFAIL;
                    rtmgr_onPreFailed();
                    break;
                }
            }
            else
            {
                if ((now - last_rx) >= failsafe_timeout && statemachine != RTMGR_STATE_SIM)
                {
                    fail_time = now;
                    statemachine = RTMGR_STATE_PREFAIL;
                    rtmgr_onPreFailed();
                    break;
                }
            }
            break;
        case RTMGR_STATE_PREFAIL:
        case RTMGR_STATE_PREEND:
            if ((has_cmd = radio.available()) && statemachine != RTMGR_STATE_PREEND)
            {
                rtmgr_onSafe(need_full_init);
                need_full_init = false;
                statemachine = RTMGR_STATE_RUN;
                break;
            }
            else
            {
                if ((now - fail_time) < (task_interval * 10))
                {
                    rtmgr_taskPreFailed();
                }
                else
                {
                    rtmgr_onPostFailed();
                    statemachine = RTMGR_STATE_POSTFAIL;
                    need_full_init = true;
                }
            }
            break;
        case RTMGR_STATE_POSTFAIL:
        case RTMGR_STATE_POSTEND:
            if ((has_cmd = radio.available()) && statemachine != RTMGR_STATE_POSTEND)
            {
                rtmgr_onSafe(need_full_init);
                need_full_init = false;
                statemachine = RTMGR_STATE_RUN;
                break;
            }
            else
            {
                rtmgr_taskPostFailed();
            }
            break;
        case RTMGR_STATE_START:
        default:
            if ((has_cmd = radio.available()))
            {
                rtmgr_onSafe(need_full_init);
                need_full_init = false;
                statemachine = RTMGR_STATE_RUN;
            }
            break;
    }
}

void rtmgr_permEnd(void)
{
    fail_time = millis();
    statemachine = RTMGR_STATE_PREEND;
    rtmgr_onPreFailed();
}

uint8_t rtmgr_hasEnded(void)
{
    if (statemachine == RTMGR_STATE_POSTEND) {
        return 2;
    }
    else if (statemachine == RTMGR_STATE_PREEND) {
        return 1;
    }
    return 0;
}

void rtmgr_setSimulation(bool x)
{
    if (x)
    {
        if (statemachine != RTMGR_STATE_RUN && statemachine != RTMGR_STATE_SIM)
        {
            if (need_full_init) {
                rtmgr_onSafe(need_full_init);
                need_full_init = false;
            }
        }
        statemachine = RTMGR_STATE_SIM;
    }
    else if (statemachine == RTMGR_STATE_SIM)
    {
        statemachine = RTMGR_STATE_START;
    }
}
