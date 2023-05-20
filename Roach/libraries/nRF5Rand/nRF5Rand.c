#include "nRF5Rand.h"

#ifndef NRFX_RNG_CONFIG_ERROR_CORRECTION
#define NRFX_RNG_CONFIG_ERROR_CORRECTION 1
#endif

#ifndef NRFX_RNG_CONFIG_IRQ_PRIORITY
#define NRFX_RNG_CONFIG_IRQ_PRIORITY 4
#endif

static volatile uint8_t* rng_fifo;
static volatile int      rng_fifo_size;
static volatile int      rng_fifo_w;
static volatile int      rng_fifo_r;

static bool has_srand = false;
static bool need_irq;
static bool need_autorestart;

static void rng_handler(void);

void nrf5rand_init(int sz, bool use_irq, bool auto_restart)
{
    need_autorestart = auto_restart;
    need_irq = use_irq;
    rng_fifo = (uint8_t*)malloc(rng_fifo_size = sz);

    NRF_RNG->CONFIG = NRFX_RNG_CONFIG_ERROR_CORRECTION;
    NRFX_IRQ_PRIORITY_SET(RNG_IRQn, NRFX_RNG_CONFIG_IRQ_PRIORITY);
    if (need_irq) {
        NRFX_IRQ_ENABLE(RNG_IRQn);
        NRF_RNG->INTENSET = 1;
    }
    NRF_RNG->TASKS_START = 1;
}

void nrf5rand_vector(uint8_t* buf, int len)
{
    if (nrf5rand_avail() < len) {
        while (nrf5rand_avail() < len) {
            nrf5rand_task();
            yield();
        }
    }
    __disable_irq();
    int i, r = rng_fifo_r;
    for (i = 0; i < len;)
    {
        buf[i] = rng_fifo[r];
        r = (r + 1) % rng_fifo_size;
        i++;
    }
    rng_fifo_r = r;
    __enable_irq();

    if (need_autorestart)
    {
        if (nrf5rand_avail() < rng_fifo_size / 2) {
            NRF_RNG->INTENSET = 1;
        }
    }

    if (has_srand == false && len >= 4)
    {
        uint32_t* xp = (uint32_t*)buf;
        srand(*xp);
        has_srand = true;
    }
}

uint32_t nrf5rand_u32(void)
{
    uint8_t octets[4];
    uint32_t x;
    uint32_t* xp = (uint32_t*)octets;
    do
    {
        nrf5rand_vector(octets, 4);
        x = *xp;
    }
    while (x == 0);
    return x;
}

uint8_t nrf5rand_u8(void)
{
    uint8_t x;
    do
    {
        nrf5rand_vector(&x, 1);
    }
    while (x == 0);
    return x;
}

int nrf5rand_avail(void)
{
    int x;
    __disable_irq();
    x = (rng_fifo_w + rng_fifo_size - rng_fifo_r) % rng_fifo_size;
    __enable_irq();
    if (need_autorestart)
    {
        if (x < rng_fifo_size / 2) {
            NRF_RNG->INTENSET = 1;
        }
    }
    return x;
}

void nrf5rand_flush(void)
{
    __disable_irq();
    rng_fifo_w = 0;
    rng_fifo_r = 0;
    __enable_irq();
    if (need_irq || need_autorestart) {
        NRF_RNG->INTENSET = 1;
    }
}

void nrf5rand_stopIrq(void)
{
    NRF_RNG->INTENCLR = 1;
}

void nrf5rand_task(void)
{
    if (NRF_RNG->EVENTS_VALRDY) {
        rng_handler();
        NRF_RNG->EVENTS_VALRDY = 0;
    }
}

static void rng_handler(void)
{
    __disable_irq();
    uint16_t n = (rng_fifo_w + 1) % rng_fifo_size;
    if (n != rng_fifo_r)
    {
        rng_fifo[rng_fifo_w] = NRF_RNG->VALUE;
        rng_fifo_w = n;
    }
    else
    {
        NRF_RNG->INTENCLR = 1;
    }
    __enable_irq();
}

void RNG_IRQHandler(void)
{
    nrf5rand_task();
}
