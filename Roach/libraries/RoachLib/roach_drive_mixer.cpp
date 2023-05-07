#include "RoachLib.h"

void roach_drive_mix(int32_t throttle, int32_t steering, int32_t gyro_correction, uint8_t flip, int32_t* output_left, int32_t* output_right, roach_nvm_servo_t* cfg)
{
    int32_t input_limit = ROACH_SCALE_MULTIPLIER;

    steering += gyro_correction;

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

    if ((flip & ROACH_FLIP_LR) != 0) {
        int32_t temp = right;
        right = left;
        left = temp;
    }
    if ((flip & ROACH_FLIP_REV_LEFT) != 0) {
        left *= -1;
    }
    if ((flip & ROACH_FLIP_REV_RIGHT) != 0) {
        right *= -1;
    }

    right = roach_drive_applyServoParams(right, cfg);
    left  = roach_drive_applyServoParams(left, cfg);

    *output_left  = left;
    *output_right = right;
}

int32_t roach_drive_applyServoParams(int32_t spd, roach_nvm_servo_t* cfg)
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
        spd += cfg->deadzone * (spd > 0 ? 1 : -1);
        spd += cfg->trim;
        spd = roach_value_clamp(spd, cfg->limit_max, cfg->limit_min);
        return spd;
    }
}

int32_t roach_ctrl_cross_mix(int32_t throttle, int32_t steering, uint32_t mix)
{
    if (mix == 0) {
        return throttle;
    }
    int32_t x = ROACH_SCALE_MULTIPLIER - roach_value_map(abs(steering), 0, ROACH_SCALE_MULTIPLIER, 0, mix, true);
    return roach_multiply_with_scale(throttle, x);
}
