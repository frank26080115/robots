#ifndef _NRF5RAND_H_
#define _NRF5RAND_H_

#include <Arduino.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void nrf5rand_init(int sz);
uint32_t nrf5rand_u32(void);
uint8_t nrf5rand_u8(void);
void nrf5rand_vector(uint8_t* buf, int len);
int nrf5rand_avail(void);

#ifdef __cplusplus
}
#endif

#endif
