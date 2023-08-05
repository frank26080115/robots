#include "nRF52UicrWriter.h"

void nrfuw_uicrErase(void)
{
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    NRF_NVMC->ERASEUICR = 1;
    while ((NRF_NVMC->READY & NVMC_READY_READY_Msk) == 0) {
        yield();
    }
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
}

void nrfuw_uicrWrite32(uint32_t x, uint32_t* addr)
{
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    while ((NRF_NVMC->READY & NVMC_READY_READY_Msk) == 0) {
        yield();
    }
    *(volatile uint32_t *)addr = x;
    __DMB();
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
}

void nrfuw_uicrDisableNfc(void)
{
    nrfuw_uicrWrite32(0xFFFFFFFE, (uint32_t)&(NRF_UICR->NFCPINS));
}

bool nrfuw_uicrIsNfcEnabled(void)
{
    return (NRF_UICR->NFCPINS & 1) != 0;
}
