#include "NonBlockingTwi.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "nrfx_twim.h"
#include "hal/nrf_twim.h"

static uint32_t pin_scl, pin_sda;
static uint8_t tx_buff[1024 * 2];

static void twi_handler(nrfx_twim_evt_t const * p_event, void * p_context);
static nrfx_twim_t m_twi = {.p_twim = NRF_TWIM0, .drv_inst_idx = 0, };//NRFX_TWIM_INSTANCE(TWI_INSTANCE_ID);
static volatile bool m_xfer_done = true;
static volatile int  m_xfer_err = 0;
static int err_cnt = 0;

extern nrfx_err_t nrfx_twim_init(nrfx_twim_t const *        p_instance,
                                 nrfx_twim_config_t const * p_config,
                                 nrfx_twim_evt_handler_t    event_handler,
                                 void *                     p_context);

extern nrfx_err_t nrfx_twim_tx(nrfx_twim_t const * p_instance,
                               uint8_t             address,
                               uint8_t     const * p_data,
                               size_t              length,
                               bool                no_stop);

// implement a FIFO with a linked list

typedef struct
{
    void* next;
    uint8_t* data;
    uint8_t i2c_addr;
    int len;
}
nbtwi_node_t;

static nbtwi_node_t* head = NULL;
static nbtwi_node_t* tail = NULL;

void nbtwi_init(int scl, int sda)
{
    nrfx_twim_config_t twi_config = {
       .scl                = g_ADigitalPinMap[pin_scl = scl],
       .sda                = g_ADigitalPinMap[pin_sda = sda],
       .frequency          = NRF_TWIM_FREQ_400K,
       .interrupt_priority = 2,
    };
    nrfx_twim_init(&m_twi, &twi_config, twi_handler, NULL);
    m_xfer_done = true;
}

void nbtwi_write(uint8_t i2c_addr, uint8_t* data, int len)
{
    nbtwi_node_t* n = malloc(sizeof(nbtwi_node_t));
    if (n == NULL) {
        return;
    }
    n->next = NULL;
    n->i2c_addr = i2c_addr;
    n->data = malloc(len);
    if (n->data == NULL) {
        free(n);
        return;
    }
    memcpy(n->data, data, len);
    if (tail != NULL)
    {
        tail->next = (void*)n;
        tail = n;
    }
    if (head == NULL) {
        head = n;
        tail = n;
    }
    nbtwi_task();
}

void nbtwi_writec(uint8_t i2c_addr, uint8_t c, uint8_t* data, int len)
{
    uint8_t* tmp = malloc(len + 1);
    if (tmp) {
        tmp[0] = c;
        memcpy(&tmp[1], data, len);
        nbtwi_write(i2c_addr, tmp, len + 1);
        free(tmp);
    }
}

static void twi_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRFX_TWIM_EVT_DONE:
            m_xfer_done = true;
            break;
        default:
            m_xfer_done = true;
            m_xfer_err = (int)p_event->type;
            break;
    }
}

void nbtwi_task(void)
{
    if (m_xfer_done)
    {
        if (m_xfer_err != 0)
        {
            nrfx_twim_bus_recover(g_ADigitalPinMap[pin_scl], g_ADigitalPinMap[pin_sda]);
            m_xfer_err = 0;
            err_cnt += 1;
        }
        else
        {
            err_cnt = 0;
        }

        if (head != NULL)
        {
            memcpy(tx_buff, head->data, head->len);
            m_xfer_done = false;
            m_xfer_err = 0;
            nrfx_twim_tx(&m_twi, head->i2c_addr, tx_buff, head->len, false);
            if (tail == head)
            {
                tail = NULL;
            }
            nbtwi_node_t* next = (nbtwi_node_t*)(head->next);
            free(head->data);
            free(head);
            head = (next == NULL) ? NULL : next;
        }
    }
}

bool nbtwi_isBusy(void)
{
    return m_xfer_done && head == NULL;
}

void nbtwi_wait(void)
{
    while (nbtwi_isBusy()) {
        yield();
    }
}

bool nbtwi_hasError(bool clr)
{
    bool x = err_cnt >= 4;
    if (clr) {
        err_cnt = 0;
    }
    return x;
}
