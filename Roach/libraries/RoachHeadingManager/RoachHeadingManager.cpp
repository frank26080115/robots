#include "RoachHeadingManager.h"

RoachHeadingManager::RoachHeadingManager(uint32_t* p_timeout
#ifdef RHEADMGR_USE_MODULES
    ,nRF52RcRadio* r, RoachIMU* i, RoachPID* p
#endif
)
{
    #ifdef RHEADMGR_USE_MODULES
    radio = r;
    imu = i;
    pid = p;
    #endif
    override_timeout = p_timeout;
}

bool RoachHeadingManager::task(roach_ctrl_pkt_t* pkt
    #ifndef RHEADMGR_USE_MODULES
        , bool force_reset, int32_t steering, float yaw, uint32_t session
    #endif
    )
{
    bool need_reset =
        #ifdef RHEADMGR_USE_MODULES
            false
        #else
            force_reset
        #endif
        ;

    if (pkt->steering != 0)
    {
        need_reset = true;
    }
    else
    {
        if (prev_pkt.steering != 0)
        {
            release_timestamp = millis();
        }

        if ((millis() - release_timestamp) < (*override_timeout))
        {
            need_reset = true;
        }
    }

    #ifdef RHEADMGR_USE_MODULES
    if ((pkt->flags & ROACHPKTFLAG_GYROACTIVE) != 0) {
        need_reset = true;
    }
    #endif

    uint32_t sess = 
        #ifdef RHEADMGR_USE_MODULES
            radio->getSessionId()
        #else
            session
        #endif
            ;
    uint32_t sess_diff = sess - prev_session_id;
    if (sess_diff > 1) {
        need_reset = true;
    }
    prev_session_id = sess;

    memcpy((void*)&prev_pkt, (void*)pkt, sizeof(roach_ctrl_pkt_t));

    #ifdef RHEADMGR_USE_MODULES
    if (imu->getErrorOccured(true)) {
        need_reset = true;
    }
    #endif

    cur_head = lround(
        #ifdef RHEADMGR_USE_MODULES
        imu->euler.
        #endif
        yaw * 100.0);

    if (need_reset)
    {
        #ifdef RHEADMGR_USE_MODULES
        pid->reset();
        #endif
        angle_offset = pkt->heading - cur_head;
    }

    tgt_head = pkt->heading + angle_offset;
    ROACH_WRAP_ANGLE(cur_head, 100);
    ROACH_WRAP_ANGLE(tgt_head, 100);

    return need_reset;
}
