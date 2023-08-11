#include <RoachLib.h>

uint16_t RoachServo_calc(int32_t ctrl, roach_nvm_servo_t* cfg)
{
    // ctrl is 0 to ROACH_SCALE_MULTIPLIER
    if (ctrl == 0) {
        return roach_value_clamp(cfg->center + cfg->trim, cfg->limit_max, cfg->limit_min);;
    }
    if (cfg->scale == 0)
    {
        if (ctrl >= 0)
        {
            ctrl = roach_value_map(ctrl, 0, ROACH_SCALE_MULTIPLIER, cfg->deadzone, cfg->limit_max - cfg->center, false);
        }
        else
        {
            ctrl = -roach_value_map(-ctrl, 0, ROACH_SCALE_MULTIPLIER, cfg->deadzone, (-cfg->limit_min) - cfg->center, false);
        }
    }
    else
    {
        ctrl = roach_multiply_with_scale(ctrl, cfg->scale);
        ctrl += cfg->deadzone * (ctrl >= 0 ? 1 : -1);
    }
    ctrl += cfg->trim;
    ctrl += cfg->center;
    ctrl = roach_value_clamp(ctrl, cfg->limit_max, cfg->limit_min);
    return ctrl;
}
