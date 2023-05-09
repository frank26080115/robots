#ifndef _NONBLOCKINGTWI_H_
#define _NONBLOCKINGTWI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void nbtwi_init(int pin_scl, int pin_sda);
void nbtwi_write(uint8_t i2c_addr, uint8_t* data, int len);
void nbtwi_writec(uint8_t i2c_addr, uint8_t c, uint8_t* data, int len);
void nbtwi_task(void);
bool nbtwi_isBusy(void);
bool nbtwi_hasError(bool);
void nbtwi_wait(void);
void nbtwi_transfer(void);

#ifdef __cplusplus
}
#endif

#endif
