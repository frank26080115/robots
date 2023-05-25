#include "nRF52RcRadio.h"
#include "nRF5Rand.h"
#include <stdlib.h>

#include "nrf_radio.h"
#include "nrf_ppi.h"
#include "nrf_egu.h"

// extern uint32_t crc32_calc(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc);
// copied from nRF5 SDK
static uint32_t crc32_compute(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc)
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

// implements a checksum using whatever method is available
uint32_t nrfrr_getCheckSum(uint32_t salt, uint8_t* data, uint8_t len)
{
    return crc32_compute((uint8_t const *)data, (uint32_t)len, (uint32_t const *)&salt);
}

uint32_t nrfrr_rand(void)
{
    #ifdef NRFRR_USE_NRF_RNG
    if (nrf5rand_avail() >= 4) {
        return nrf5rand_u32();
    }
    else
    #endif
    {
        return rand();
    }
}

void nrfrr_fakeCipher(uint8_t* buf, uint32_t x, uint32_t y)
{
    uint32_t* tmp32 = (uint32_t*)buf;
    tmp32[0] = x;
    tmp32[1] = y != 0 ? y : x;
    tmp32[2] = nrfrr_getCheckSum(0, (uint8_t*)tmp32, 8);
    tmp32[3] = nrfrr_getCheckSum(0, (uint8_t*)&(tmp32[1]), 8);
}
