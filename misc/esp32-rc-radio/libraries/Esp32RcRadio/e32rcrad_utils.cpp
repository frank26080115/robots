#include <stdint.h>
#include <esp32/rom/crc.h>
#include <esp_system.h>

// shift left with rollover
uint32_t _rotl_u32(uint32_t value, int shift)
{
    shift %= 32;
    if (shift == 0) {
        return value;
    }
    return (value << shift) | (value >> (32 - shift));
}

// implements a checksum using whatever method is available
uint32_t esp32rcrad_getCheckSum(uint32_t salt, uint8_t* data, uint8_t len)
{
    return crc32_le(salt, (uint8_t const *)data, (uint32_t)len);
    //return 0;
}

#if 0

// below is a failed attempt at using https://github.com/Jeija/esp32free80211
// problem is getting the correct versions of everything needed, and integrate that into Arduino
// the benefit would have been a better filter on the promiscuous callback, using block-ack packets

#ifdef __cplusplus
extern "C" {
#endif
int8_t ieee80211_freedom_output(uint32_t *ifx, uint8_t *buffer, uint16_t len);
#ifdef __cplusplus
}
#endif

static uint32_t zeros[64] = { 0x00000000 };
extern void* g_wifi_menuconfig;

int8_t free80211_send(uint8_t *buffer, uint16_t len)
{
    int8_t rval = 0;

    *(uint32_t *)((uint32_t)&g_wifi_menuconfig - 0x253) = 1;
    rval = ieee80211_freedom_output(zeros, buffer, len);
    if (rval == -4) asm("ill");
    return rval;
}
#endif
