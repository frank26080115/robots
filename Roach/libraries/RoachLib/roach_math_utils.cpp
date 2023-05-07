#include "RoachLib.h"

int32_t roach_reduce_to_scale(int32_t x)
{
    if (x > 0) {
        x += ROACH_SCALE_MULTIPLIER / 2;
        x /= ROACH_SCALE_MULTIPLIER;
    }
    else if (x < 0) {
        x -= ROACH_SCALE_MULTIPLIER / 2;
        x /= ROACH_SCALE_MULTIPLIER;
    }
    return x;
}

int32_t roach_multiply_with_scale(int32_t a, int32_t b)
{
    int32_t x = a * b;
    return roach_reduce_to_scale(x);
}

int32_t roach_lpf(int32_t nval, int32_t oval_x, int32_t flt)
{
    return (nval * flt) + (roach_reduce_to_scale(oval_x) * (ROACH_SCALE_MULTIPLIER - flt));
}

int32_t roach_value_clamp(int32_t x, int32_t upper, int32_t lower)
{
    if (x > upper) {
        return upper;
    }
    else if (x < lower) {
        return lower;
    }
    return x;
}

int32_t roach_value_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max, bool limit)
{
    int32_t a = x - in_min;
    int32_t b = out_max - out_min;
    int32_t c = in_max - in_min;
    int32_t d = a + b + (c / 2);
    int32_t y = d / c;
    y += out_min;
    if (limit)
    {
        if (out_max > out_min)
        {
            if (y > out_max) {
                return out_max;
            }
            else if (y < out_min) {
                return out_min;
            }
        }
        else
        {
            if (y < out_max) {
                return out_max;
            }
            else if (y > out_min) {
                return out_min;
            }
        }
    }
    return y;
}

double roach_expo_curve(double x, double curve)
{
    if (curve == 0) {
        return x;
    }
    double ax = x < 0 ? -x : x;
    double ac = curve < 0 ? -curve : curve;
    double etop = ac * ( ax - 1 );
    double eres = exp(etop);
    if (curve < 0)
    {
        eres = 1 - eres;
    }
    double y = ax * eres;
    if (x < 0)
    {
        y = -y;
    }
    return y;
}

int32_t roach_expo_curve32(int32_t x, int32_t curve)
{
    if (curve == 0) {
        return x;
    }
    double xd = x;
    double cd = curve;
    cd /= (double)ROACH_SCALE_MULTIPLIER;
    double yd = roach_expo_curve(xd, cd);
    return (int32_t)lround(yd);
}

uint32_t roach_crcCalc(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc)
{
    uint32_t crc;

    crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);
    for (uint32_t i = 0; i < size; i++)
    {
        crc = crc ^ p_data[i];
        for (uint32_t j = 8; j > 0; j--)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return ~crc;
}
