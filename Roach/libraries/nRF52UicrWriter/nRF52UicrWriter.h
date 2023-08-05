#ifndef _NRF52UICRWRITER_H_
#define _NRF52UICRWRITER_H_

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

void nrfuw_uicrErase(void);
void nrfuw_uicrWrite32(uint32_t x, uint32_t* addr);
void nrfuw_uicrDisableNfc(void);
bool nrfuw_uicrIsNfcEnabled(void);

#ifdef __cplusplus
}
#endif

#endif
