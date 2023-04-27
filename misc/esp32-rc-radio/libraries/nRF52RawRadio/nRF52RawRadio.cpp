#include "nRF52RawRadio.h"

void RADIO_IRQHandler_cpp(void)
{
    
}

#ifdef __cplusplus
extern "C" {
#endif
void RADIO_IRQHandler(void)
{
    RADIO_IRQHandler_cpp();
}
#ifdef __cplusplus
}
#endif