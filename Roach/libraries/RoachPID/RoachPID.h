#ifndef _ROACHPID_H_
#define _ROACHPID_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include <RoachLib.h>

class RoachPID
{
    public:
        RoachPID(void);
        reset(void);
        roach_nvm_pid_t* cfg;
        int32_t compute(int32_t x, int32_t tgt);
        void debug(void);

    private:
        int32_t last_diff;
        int32_t accumulator;

        // these are more like local variables but kept in memory for debugging
        int32_t _last_cur, _last_tgt;
        int32_t _diff, _p, _i, _d, _delta_e, _output;
        uint32_t t;
};

#endif
