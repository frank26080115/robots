#include "RoachRobotTaskManager.h"
#include "RoachRobotPrivate.h"

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
    RTMGR_STATE_PREEND,
    RTMGR_STATE_POSTEND,
    RTMGR_STATE_SIM,
};

static uint8_t statemachine = RTMGR_STATE_INIT;

void rtmgr_init(uint32_t intv, uint32_t timeout)
{
    task_interval = intv;
    failsafe_timeout = timeout;
    statemachine = RTMGR_STATE_START;
}

bool rtmgr_task(uint32_t now)
{
    bool has_called = false;
    bool has_cmd;
    if (statemachine == RTMGR_STATE_RUN || statemachine == RTMGR_STATE_SIM)
    {
        if ((has_cmd = radio.available()) || (now - last_time) >= (task_interval + (task_interval / 2)))
        {
            last_time = now;
            last_rx = (has_cmd || statemachine == RTMGR_STATE_SIM) ? now : last_rx;
            rtmgr_taskPeriodic(has_cmd);
            has_called = true;
        }
    }

    switch (statemachine)
    {
        case RTMGR_STATE_RUN:
        case RTMGR_STATE_SIM:
            {
                bool enter_failsafe = false;
                if (failsafe_timeout == 0) {
                    enter_failsafe = (radio.isConnected() == false && statemachine != RTMGR_STATE_SIM);
                }
                else {
                    enter_failsafe = ((now - last_rx) >= failsafe_timeout && statemachine != RTMGR_STATE_SIM);
                }
                if (enter_failsafe)
                {
                    fail_time = now;
                    statemachine = RTMGR_STATE_PREFAIL;
                    debug_printf("[%u] rtmgr_onPreFailed\r\n", now);
                    rtmgr_onPreFailed();
                    has_called = true;
                    break;
                }
            }
            break;
        case RTMGR_STATE_PREFAIL:
        case RTMGR_STATE_PREEND:
            if ((has_cmd = radio.available()) && statemachine != RTMGR_STATE_PREEND)
            {
                debug_printf("[%u] rtmgr_onSafe from pre\r\n", now);
                rtmgr_onSafe(need_full_init);
                need_full_init = false;
                statemachine = RTMGR_STATE_RUN;
                has_called = true;
                break;
            }
            else
            {
                if ((now - fail_time) < (task_interval * 20))
                {
                    if ((now - last_time) >= task_interval) {
                        last_time = now;
                        rtmgr_taskPreFailed();
                        has_called = true;
                    }
                }
                else
                {
                    debug_printf("[%u] rtmgr_onPostFailed\r\n", now);
                    rtmgr_onPostFailed();
                    statemachine = RTMGR_STATE_POSTFAIL;
                    need_full_init = true;
                    has_called = true;
                }
            }
            break;
        case RTMGR_STATE_POSTFAIL:
        case RTMGR_STATE_POSTEND:
            if ((has_cmd = radio.available()) && statemachine != RTMGR_STATE_POSTEND)
            {
                debug_printf("[%u] rtmgr_onSafe from post\r\n", now);
                rtmgr_onSafe(need_full_init);
                has_called = true;
                need_full_init = false;
                statemachine = RTMGR_STATE_RUN;
                break;
            }
            else
            {
                if ((now - last_time) >= task_interval) {
                    last_time = now;
                    rtmgr_taskPostFailed();
                    has_called = true;
                }
            }
            break;
        case RTMGR_STATE_START:
        default:
            if ((has_cmd = radio.available()))
            {
                debug_printf("[%u] rtmgr_onSafe from init state\r\n", now);
                rtmgr_onSafe(need_full_init);
                has_called = true;
                need_full_init = false;
                statemachine = RTMGR_STATE_RUN;
            }
            else
            {
                if ((now - last_time) >= task_interval)
                {
                    last_time = now;
                    rtmgr_taskPreStart();
                    has_called = true;
                }
            }
            break;
    }
    return has_called;
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
                debug_printf("[%u] rtmgr_onSafe from rtmgr_setSimulation\r\n", millis());
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
