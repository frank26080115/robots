#include <stdint.h>
#include <esp32/rom/crc.h>
#include <esp_system.h>

// implements a checksum using whatever method is available
uint32_t e32rcrad_getCheckSum(uint32_t salt, uint8_t* data, uint8_t len)
{
    return crc32_le(salt, (uint8_t const *)data, (uint32_t)len);
    //return 0;
}
