#ifndef _ROACHHEADINGMANAGER_H_
#define _ROACHHEADINGMANAGER_H_

#include <RoachLib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

//#define RHEADMGR_USE_MODULES

#ifdef RHEADMGR_USE_MODULES
#include <nRF52RcRadio.h>
#include <RoachIMU.h>
#include <RoachPID.h>
#endif

class RoachHeadingManager
{
    public:
        RoachHeadingManager(uint32_t* p_timeout
            #ifdef RHEADMGR_USE_MODULES
                nRF52RcRadio* r, RoachIMU* i, RoachPID* p
            #endif
            );
        bool task(roach_ctrl_pkt_t* pkt
            #ifndef RHEADMGR_USE_MODULES
                , bool force_reset, int32_t steering, float yaw, uint32_t session
            #endif
            );
        inline int32_t getTgtHeading(void)  { return tgt_head; };
        inline int32_t getCurHeading(void)  { return cur_head; };
        inline int32_t getOffsetAngle(void) { return angle_offset; };

    private:
        #ifdef RHEADMGR_USE_MODULES
        nRF52RcRadio* radio;
        RoachIMU* imu;
        RoachPID* pid;
        #endif

        uint32_t* override_timeout;

        int32_t angle_offset;
        int32_t cur_head;
        int32_t tgt_head;
        roach_ctrl_pkt_t prev_pkt;
        uint32_t prev_session_id = 0;
        uint32_t release_timestamp = 0;
};

#endif
