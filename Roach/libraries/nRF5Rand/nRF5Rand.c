#include "nRF5Rand.h"
#include "nrfx_rng.h"

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
static bool need_autorestart;

static void rng_handler(uint8_t rng_data);

void nrf5rand_init(int sz, bool use_irq, bool auto_restart)
{
    need_autorestart = auto_restart;
    rng_fifo = (uint8_t*)malloc(rng_fifo_size = sz);
    static nrfx_rng_config_t cfg = NRFX_RNG_DEFAULT_CONFIG;
    nrfx_rng_init(&cfg, rng_handler);
    if (use_irq) {
        nrfx_rng_start();
    }
    else {
        #ifdef NRF_RNG_USE_NEW_VERSION
        nrf_rng_task_trigger(NRF_RNG_TASK_START);
        #else
        nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);
        #endif
    }
}

void nrf5rand_vector(uint8_t* buf, int len)
{
    if (nrf5rand_avail() < len) {
        nrfx_rng_start();
        while (nrf5rand_avail() < len) {
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
            nrfx_rng_start();
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
            nrfx_rng_start();
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
    nrfx_rng_start();
}

void nrf5rand_stopIrq(void)
{
    #ifdef NRF_RNG_USE_NEW_VERSION
    nrf_rng_int_disable(NRF_RNG_INT_VALRDY_MASK);
    #else
    nrf_rng_int_disable(NRF_RNG, NRF_RNG_INT_VALRDY_MASK);
    #endif
}

void nrf5rand_task(void)
{
    if (NRF_RNG->EVENTS_VALRDY) {
        rng_handler(nrf_rng_random_value_get(NRF_RNG));
        #ifdef NRF_RNG_USE_NEW_VERSION
        nrf_rng_event_clear(NRF_RNG_EVENT_VALRDY);
        #else
        nrf_rng_event_clear(NRF_RNG, NRF_RNG_EVENT_VALRDY);
        #endif
    }
}

static void rng_handler(uint8_t rng_data)
{
    uint16_t n = (rng_fifo_w + 1) % rng_fifo_size;
    if (n != rng_fifo_r)
    {
        rng_fifo[rng_fifo_w] = rng_data;
        rng_fifo_w = n;
    }
    else
    {
        nrfx_rng_stop();
    }
}
