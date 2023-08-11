#include "RoachDriveMixer.h"
#include <RoachLib.h>

RoachDriveMixer::RoachDriveMixer(void)
{
}

void RoachDriveMixer::mix(int32_t throttle, int32_t steering, int32_t gyro_correction)
{
    int32_t input_limit = ROACH_SCALE_MULTIPLIER;

    throttle = calcCrossMixThrottle(throttle, steering);

    steering += gyro_correction;
    steering = roach_value_clamp_abs(steering, input_limit);

    // https://home.kendra.com/mauser/joystick.html
    int32_t inv_steering = -steering;
    int32_t v = input_limit - abs(inv_steering);
    v *= input_limit;
    v *= throttle;
    v += throttle * input_limit;
    int32_t w = input_limit - abs(throttle);
    w *= input_limit;
    w *= inv_steering;
    w += inv_steering * input_limit;
    int32_t right = roach_reduce_to_scale((v + w) / 2);
    int32_t left  = roach_reduce_to_scale((v - w) / 2);

    _raw_left  = left;
    _raw_right = right;

    if ((_flip & ROACH_FLIP_LR) != 0) {
        int32_t temp = right;
        right = left;
        left = temp;
    }
    if ((_flip & ROACH_FLIP_REV_LEFT) != 0) {
        left *= -1;
    }
    if ((_flip & ROACH_FLIP_REV_RIGHT) != 0) {
        right *= -1;
    }
    if (_upsidedown) {
        left *= -1;
        right *= -1;
        int32_t tmpswap = right;
        right = left;
        left = tmpswap;
    }

    right = applyServoParams(cfg_left, right);
    left  = applyServoParams(cfg_right, left);

    _result_left  = left;
    _result_right = right;
}

int32_t RoachDriveMixer::applyServoParams(roach_nvm_servo_t* cfg, int32_t spd)
{
    if (cfg == NULL) {
        return spd;
    }

    if (spd == 0) {
        return cfg->center + cfg->trim;
    }
    else
    {
        spd *= cfg->scale;
        spd = roach_reduce_to_scale(spd);
        spd += cfg->deadzone * (spd >= 0 ? 1 : -1);
        spd += cfg->trim;
        spd = roach_value_clamp(spd, cfg->limit_max, cfg->limit_min);
        return spd;
    }
}

int32_t RoachDriveMixer::calcCrossMixThrottle(int32_t throttle, int32_t steering)
{
    // slow down the robot if steering is hard
    if (cfg_crossmix == NULL) {
        return throttle;
    }
    if (*cfg_crossmix == 0) {
        return throttle;
    }
    int32_t x = ROACH_SCALE_MULTIPLIER - roach_value_map(abs(steering), 0, ROACH_SCALE_MULTIPLIER, 0, *cfg_crossmix, true);
    return roach_multiply_with_scale(throttle, x);
}

RoachVirtualHeading::RoachVirtualHeading(void)
{
}

int32_t RoachVirtualHeading::track(int32_t left, int32_t right)
{
    uint32_t now = millis();
    if ((now - _last_time) < 10) {
        return _heading;
    }
    _last_time = now;
    applyAccel(left, &_left);
    applyAccel(right, &_right);
    int32_t diff = _left - _right;
    if (cfg != NULL && cfg->limit_diff > 0) {
        diff = diff >  cfg->limit_diff ?  cfg->limit_diff : diff;
        diff = diff < -cfg->limit_diff ? -cfg->limit_diff : diff;
    }
    _accum += diff;
    if (cfg != NULL && cfg->limit_rot > 0)
    {
        while (_accum > cfg->limit_rot) {
            _accum -= cfg->limit_rot;
        }
        while (_accum < -cfg->limit_rot) {
            _accum += cfg->limit_rot;
        }

        if (_accum >= 0) {
            _heading = roach_value_map(_accum, 0, cfg->limit_rot, 0, 3600, false);
        }
        else {
            _heading = -roach_value_map(-_accum, 0, cfg->limit_rot, 0, 3600, false);
        }

        while (_heading > 1800) {
            _heading -= 3600;
        }
        while (_heading < -1800) {
            _heading += 3600;
        }
    }
    else
    {
        _heading = _accum;
    }

    return _heading;
}

void RoachVirtualHeading::applyAccel(int32_t in, int32_t* outp)
{
    if (cfg == NULL) {
        *outp = in;
        return;
    }

    int32_t out = *outp;
    if (in >= 0 && out >= in)
    {
        out += cfg->accel;
        out = out > in ? in : out;
    }
    else if (in >= 0 && out < in)
    {
        out -= cfg->deccel;
        out = out < in ? in : out;
    }
    else if (in < 0 && out <= in)
    {
        out -= cfg->accel;
        out = out < in ? in : out;
    }
    else if (in < 0 && out > in)
    {
        out += cfg->deccel;
        out = out > in ? in : out;
    }
    *outp = out;
}

void RoachVirtualHeading::reset(void)
{
    _accum = 0;
    _heading = 0;
    _last_time = 0;
}
