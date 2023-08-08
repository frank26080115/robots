#include "RoachRobotTaskManager.h"

static uint32_t task_interval;
static uint32_t failsafe_timeout;
static uint32_t last_time = 0;
static uint32_t last_rx = 0;
static uint32_t fail_time = 0;

enum
{
    RTMGR_STATE_INIT,
    RTMGR_STATE_START,
    RTMGR_STATE_RUN,
    RTMGR_STATE_PREFAIL,
    RTMGR_STATE_POSTFAIL,
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
    if (statemachine == RTMGR_STATE_RUN)
    {
        if ((has_cmd = radio.available()) || (now - last_time) >= (task_interval + (task_interval / 2)))
        {
            last_time = now;
            last_rx = has_cmd ? now : last_rx;
            rtmgr_taskPeriodic(has_cmd);
        }
    }

    switch (statemachine)
    {
        case RTMGR_STATE_RUN:
            if (failsafe_timeout == 0)
            {
                if (radio.isConnected() == false)
                {
                    fail_time = now;
                    statemachine = RTMGR_STATE_PREFAIL;
                    rtmgr_onPreFailed();
                    break;
                }
            }
            else
            {
                if ((now - last_rx) >= failsafe_timeout)
                {
                    fail_time = now;
                    statemachine = RTMGR_STATE_PREFAIL;
                    rtmgr_onPreFailed();
                    break;
                }
            }
            break;
        case RTMGR_STATE_PREFAIL:
            if ((has_cmd = radio.available()))
            {
                rtmgr_onSafe(false);
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
                }
            }
            break;
        case RTMGR_STATE_POSTFAIL:
            if ((has_cmd = radio.available()))
            {
                rtmgr_onSafe(true);
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
                rtmgr_onSafe(true);
                statemachine = RTMGR_STATE_RUN;
            }
            break;
    }
}
