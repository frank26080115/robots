#ifndef _NONBLOCKINGTWI_H_
#define _NONBLOCKINGTWI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void nbtwi_init(int pin_scl, int pin_sda, int bufsz);
void nbtwi_write(uint8_t i2c_addr, uint8_t* data, int len, bool no_stop);
void nbtwi_writec(uint8_t i2c_addr, uint8_t c, uint8_t* data, int len);
void nbtwi_read(uint8_t i2c_addr, int len);
void nbtwi_task(void);
bool nbtwi_isBusy(void);
bool nbtwi_hasError(bool);
int nbtwi_lastError(void);
bool nbtwi_hasResult(void);
bool nbtwi_readResult(uint8_t* data, int len, bool clr);
void nbtwi_transfer(void);

#ifdef __cplusplus
}
#endif

#endif
