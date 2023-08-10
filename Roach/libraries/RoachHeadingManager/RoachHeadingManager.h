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

        // yaw angle specified in real degrees
        bool task(roach_ctrl_pkt_t* pkt
            #ifndef RHEADMGR_USE_MODULES
                , float yaw, uint32_t session
            #endif
            );

        // this should be triggered by events such as radio loss or IMU falure
        // resets the angle tracking
        inline void    setReset      (void) { pending_reset = true; };

        // all outputs are -1800 to 1800 (degree * ROACH_ANGLE_MULTIPLIER)
        inline int32_t getTgtHeading (void) { return tgt_head; };      // target  angle for the PID controller
        inline int32_t getCurHeading (void) { return cur_head; };      // current angle for the PID controller
        inline int32_t getOffsetAngle(void) { return angle_offset; };  // difference between the human input heading and the target angle heading

    private:
        #ifdef RHEADMGR_USE_MODULES
        nRF52RcRadio* radio;
        RoachIMU* imu;
        RoachPID* pid;
        #endif

        uint32_t* override_timeout;

        int32_t angle_offset; // difference between the human input heading and the target angle heading
        int32_t cur_head;     // current angle for the PID controller
        int32_t tgt_head;     // target  angle for the PID controller
        roach_ctrl_pkt_t prev_pkt;
        uint32_t prev_session_id = 0;
        uint32_t release_timestamp = 0;
        bool pending_reset;
};

#endif
