#include "RoachPID.h"

RoachPID::RoachPID(void)
{
    reset();
}

void RoachPID::reset(void)
{
    accumulator = 0;
    last_diff = 0;
}

int32_t RoachPID::compute(int32_t cur, int32_t tgt)
{
    t = millis();

    _last_cur = cur;
    _last_tgt = cur;

    _diff = tgt - cur;
    ROACH_WRAP_ANGLE(_diff, ROACH_ANGLE_MULTIPLIER);

    _p = cfg->p * _diff;

    if ((_diff >= 0 && _diff <= (90 * ROACH_ANGLE_MULTIPLIER) && last_diff < 0) || (_diff < 0 && _diff >= -(90 * ROACH_ANGLE_MULTIPLIER) && last_diff >= 0)) {
        // spun past target, accumulator should be 0 or pushing towards target, not away
        accumulator = (accumulator >= 0 && _diff < 0) || (accumulator < 0 && _diff >= 0) ? 0 : accumulator;
    }

    accumulator += _diff;
    accumulator = accumulator > cfg->accumulator_limit ? cfg->accumulator_limit : (accumulator < -cfg->accumulator_limit ? -cfg->accumulator_limit : accumulator);

    _i = cfg->i * roach_div_rounded(accumulator, ROACH_ANGLE_MULTIPLIER);
    // I want more resolution for the I term, so remove the decimal places in the accumulated error

    if (accumulator > cfg->accumulator_decay) {
        accumulator -= cfg->accumulator_decay;
    }
    else if (accumulator < -cfg->accumulator_decay) {
        accumulator += cfg->accumulator_decay;
    }
    else {
        accumulator = 0;
    }

    if ((_diff >= 0 && last_diff <= -(90 * ROACH_ANGLE_MULTIPLIER)) || (_diff < 0 && last_diff >= (90 * ROACH_ANGLE_MULTIPLIER))) {
        // rotated past 180
        // accumulator should follow P term
        if ((accumulator < 0 && _p >= 0) || (accumulator > 0 && _p < 0)) {
            accumulator *= -1;
        }
    }

    int32_t abs_e_n = _diff < 0 ? -_diff : _diff;
    int32_t abs_e_o = last_diff < 0 ? -last_diff : last_diff;
    int32_t abs_de, abs_td;
    if (abs_e_n < abs_e_o)
    {
        // approaching target
        abs_de = abs_e_o - abs_e_n;
        abs_td = abs_de * cfg->d;
        // D term should counter P term
        _d = _p < 0 ? abs_td : -abs_td;
    }
    else
    {
        // leaving target
        abs_de = abs_e_n - abs_e_o;
        abs_td = abs_de * cfg->d;
        // D term should follow P term
        _d = _p < 0 ? -abs_td : abs_td;
    }

    last_diff = _diff;

    int32_t actual_out = roach_reduce_to_scale(_output = (_p + _i + _d));
    return _last_out = roach_value_clamp_abs(actual_out, cfg->output_limit);
}

void RoachPID::debug(void)
{
    Serial.printf("PID[%u]: "
        "%d , "
        "%d , "
        "%d , "
        "%d , "
        "%d , "
        "%d , "
        "%d , "
        "%d , "
        "%d , "
        "\r\n",
        millis(),
        _last_cur,
        _last_tgt,
        _diff,
        _p,
        accumulator,
        _i,
        _delta_e,
        _d,
        _output
        );
}
