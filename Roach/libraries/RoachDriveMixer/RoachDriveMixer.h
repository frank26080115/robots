#ifndef _ROACHDRIVEMIXER_H_
#define _ROACHDRIVEMIXER_H_

#include <RoachLib.h>
#include <stdint.h>
#include <stdbool.h>

class RoachDriveMixer
{
    public:
        RoachDriveMixer(void);
        void mix(int32_t throttle, int32_t steering, int32_t gyro_correction);
        inline int32_t getLeft (void) { return _result_left;  };
        inline int32_t getRight(void) { return _result_right; };
        void setCrossMix(int32_t x) { _crossmix = x; };
        roach_nvm_servo_t* cfg_left;
        roach_nvm_servo_t* cfg_right;
        inline void setFlip(uint8_t x) { _flip = x; };
        inline void setUpsideDown(uint8_t x) { _upsidedown = x; };

    private:
        int32_t _result_left = 0, _result_right = 0;
        int32_t _raw_left = 0, _raw_right = 0;
        uint8_t _flip = 0;
        bool    _upsidedown = false;
        int32_t _crossmix = 0;
        int32_t _virtual_heading;
        int32_t applyServoParams(roach_nvm_servo_t* cfg, int32_t x);
        int32_t calcCrossMixThrottle(int32_t throttle, int32_t steering);
};

class RoachVirtualHeading
{
    public:
        RoachVirtualHeading(void);
        int32_t track(int32_t left, int32_t right);
        inline int32_t get(void) { return _heading; };
        roach_nvm_virheading_t* cfg = NULL;
        void reset(void);
    private:
        uint32_t _last_time = 0;
        int32_t _accum = 0;
        int32_t _heading = 0;
        int32_t _left = 0, _right = 0;

        void applyAccel(int32_t in, int32_t* outp);
};

#endif
