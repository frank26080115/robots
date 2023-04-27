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

    _p = cfg->p * diff;
    
    accumulator += diff;
    accumulator = accumulator > cfg->accumulator_limit ? cfg->accumulator_limit : (accumulator < -cfg->accumulator_limit ? -cfg->accumulator_limit : accumulator);
    _i = cfg->i * accumulator;
    if (accumulator > cfg->accumulator_decay) {
        accumulator -= cfg->accumulator_decay;
    }
    else if (accumulator < -cfg->accumulator_decay) {
        accumulator += cfg->accumulator_decay;
    }
    else {
        accumulator = 0;
    }

    _delta_e = diff - last_diff;
    last_diff = diff;
    _d = cfg->d * delta_e;

    return roach_reduce_to_scale(_output = (_p + _i + _d));
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
        "\r\n"
        t,
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
