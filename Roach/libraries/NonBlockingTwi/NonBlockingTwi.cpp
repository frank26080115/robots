#include "NonBlockingTwi.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "nrfx_twim.h"
#include "hal/nrf_twim.h"

enum
{
    NBTWI_SM_IDLE,
    NBTWI_SM_STARTED,
    NBTWI_SM_WAIT,
    NBTWI_SM_STOP,
    NBTWI_SM_ERROR,
};

static uint32_t pin_scl, pin_sda;
static uint8_t tx_buff[1024 * 2];

static volatile uint8_t  nbtwi_statemachine = 0;
static volatile bool     xfer_done = true;
static volatile uint32_t xfer_err = 0;
static int err_cnt = 0;


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

static volatile uint32_t* pincfg_reg(uint32_t pin)
{
    NRF_GPIO_Type * port = nrf_gpio_pin_port_decode(&pin);
    return &port->PIN_CNF[pin];
}

void nbtwi_init(int scl, int sda)
{
    *pincfg_reg(g_ADigitalPinMap[pin_scl = scl]) = ((uint32_t)GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_PULL_Pullup      << GPIO_PIN_CNF_PULL_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0D1       << GPIO_PIN_CNF_DRIVE_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);

    *pincfg_reg(g_ADigitalPinMap[pin_sda = sda]) = ((uint32_t)GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_PULL_Pullup      << GPIO_PIN_CNF_PULL_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0D1       << GPIO_PIN_CNF_DRIVE_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);

    NRF_TWIM0->FREQUENCY = TWIM_FREQUENCY_FREQUENCY_K400;
    NRF_TWIM0->ENABLE    = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);
    NRF_TWIM0->PSEL.SCL  = g_ADigitalPinMap[pin_scl];
    NRF_TWIM0->PSEL.SDA  = g_ADigitalPinMap[pin_sda];

    //uint32_t irqn = nrfx_get_irq_number(NRF_TWIM0);
    //NVIC_ClearPendingIRQ(irqn);
    //NVIC_SetPriority(irqn, 3);
    //NVIC_EnableIRQ(irqn);

    #ifdef NBTWI_ENABLE_DEBUG
    Serial.println("NonBlockingTwi begin");
    #endif
}

void nbtwi_write(uint8_t i2c_addr, uint8_t* data, int len)
{
    nbtwi_node_t* n = (nbtwi_node_t*)malloc(sizeof(nbtwi_node_t));
    if (n == NULL) {
        return;
    }
    n->next = NULL;
    n->i2c_addr = i2c_addr;
    n->data = (uint8_t*)malloc(len);
    if (n->data == NULL) {
        free(n);
        return;
    }
    memcpy(n->data, data, len);
    n->len = len;
    if (tail != NULL)
    {
        tail->next = (void*)n;
        tail = n;
    }
    if (head == NULL) {
        head = n;
        tail = n;
    }
    if (xfer_done) {
        nbtwi_task();
    }
}

#ifdef NBTWI_ENABLE_DEBUG
extern uint32_t getFreeRam(void);
#endif

void nbtwi_writec(uint8_t i2c_addr, uint8_t c, uint8_t* data, int len)
{
    #ifdef NBTWI_ENABLE_DEBUG
    Serial.printf("nbtwi_writec 0x%02X 0x%02X %u ", i2c_addr, c, len);
    if (len == 1)
    {
        Serial.printf("0x%02X", data[0]);
    }
    Serial.printf(" ... ");
    #endif
    nbtwi_node_t* n = (nbtwi_node_t*)malloc(sizeof(nbtwi_node_t));
    if (n == NULL) {
        return;
    }
    n->next = NULL;
    n->i2c_addr = i2c_addr;
    n->data = (uint8_t*)malloc(len + 1);
    if (n->data == NULL) {
        free(n);
        return;
    }
    n->data[0] = c;
    n->len = len + 1;
    memcpy(&(n->data[1]), data, len);

    #ifdef NBTWI_ENABLE_DEBUG
    int i;
    for (i = 0; i < n->len; i++) {
        Serial.printf(" 0x%02X", n->data[i]);
    }
    #endif

    if (tail != NULL)
    {
        tail->next = (void*)n;
        tail = n;
    }
    if (head == NULL) {
        head = n;
        tail = n;
    }
    #ifdef NBTWI_ENABLE_DEBUG
    Serial.printf(" done, mem %u\r\n", getFreeRam());
    delay(20);
    #endif
    if (xfer_done) {
        nbtwi_task();
    }
}

static void nbtwi_runStateMachine(void)
{
    if (nbtwi_statemachine == NBTWI_SM_STARTED)
    {
        if (NRF_TWIM0->EVENTS_ERROR)
        {
            NRF_TWIM0->EVENTS_ERROR = 0;
            xfer_err = NRF_TWIM0->ERRORSRC;
            NRF_TWIM0->ERRORSRC = xfer_err;
            NRF_TWIM0->TASKS_STOP = 0x1UL;
            nbtwi_statemachine = NBTWI_SM_ERROR;
        }
        else if (NRF_TWIM0->EVENTS_TXSTARTED)
        {
            NRF_TWIM0->EVENTS_TXSTARTED = 0;
            nbtwi_statemachine = NBTWI_SM_WAIT;
        }
    }
    if (nbtwi_statemachine == NBTWI_SM_WAIT)
    {
        if (NRF_TWIM0->EVENTS_ERROR)
        {
            NRF_TWIM0->EVENTS_ERROR = 0;
            xfer_err = NRF_TWIM0->ERRORSRC;
            NRF_TWIM0->ERRORSRC = xfer_err;
            NRF_TWIM0->TASKS_STOP = 0x1UL;
            nbtwi_statemachine = NBTWI_SM_ERROR;
        }
        else if (NRF_TWIM0->EVENTS_LASTTX)
        {
            NRF_TWIM0->EVENTS_LASTTX = 0;
            NRF_TWIM0->TASKS_STOP = 0x1UL;
            nbtwi_statemachine = NBTWI_SM_STOP;
        }
    }
    if (nbtwi_statemachine == NBTWI_SM_STOP || nbtwi_statemachine == NBTWI_SM_ERROR)
    {
        if (NRF_TWIM0->EVENTS_STOPPED)
        {
            NRF_TWIM0->EVENTS_STOPPED = 0;
            nbtwi_statemachine = NBTWI_SM_IDLE;
            xfer_done = true;
        }
    }
}

#ifdef __cplusplus
extern "C" {
#endif
//void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void)
//{
//    nbtwi_runStateMachine();
//}
#ifdef __cplusplus
}
#endif

void nbtwi_task(void)
{
    nbtwi_runStateMachine();
    if (xfer_done)
    {
        if (xfer_err != 0)
        {
            #ifdef NBTWI_ENABLE_BUS_RECOVER
            nrfx_twi_twim_bus_recover(g_ADigitalPinMap[pin_scl], g_ADigitalPinMap[pin_sda]);
            #endif
            #ifdef NBTWI_ENABLE_DEBUG
            Serial.printf("nbtwi err 0x%08X\r\n", xfer_err);
            #endif

            xfer_err = 0;
            err_cnt += 1;
        }
        else
        {
            err_cnt = 0;
        }

        if (head != NULL)
        {
            memcpy(tx_buff, head->data, head->len);
            xfer_done = false;
            xfer_err = 0;

            #ifdef NBTWI_ENABLE_DEBUG
            Serial.printf("nbtwi_task next packet 0x%02X %u ... ", head->i2c_addr, head->len);
            delay(20);
            #endif

            NRF_TWIM0->ADDRESS        = head->i2c_addr;
            NRF_TWIM0->EVENTS_STOPPED = 0x0UL;
            NRF_TWIM0->TASKS_RESUME   = 0x1UL;
            NRF_TWIM0->TXD.PTR        = (uint32_t)(tx_buff);
            NRF_TWIM0->TXD.MAXCNT     = (head->len);
            NRF_TWIM0->TASKS_STARTTX  = 0x1UL;
            nbtwi_statemachine = NBTWI_SM_STARTED;

            #ifdef NBTWI_ENABLE_DEBUG
            int i;
            for (i = 0; i < (head->len); i++) {
                Serial.printf(" 0x%02X", tx_buff[i]);
            }
            Serial.printf(" done\r\n");
            #endif

            if (tail == head)
            {
                tail = NULL;
            }
            nbtwi_node_t* next = (nbtwi_node_t*)(head->next);
            if (head == tail) {
                tail = NULL;
            }
            free(head->data);
            free(head);
            head = next;
        }
    }
}

bool nbtwi_isBusy(void)
{
    return xfer_done == false || head != NULL;
}

void nbtwi_transfer(void)
{
    #ifdef NBTWI_ENABLE_DEBUG
    Serial.printf("nbtwi_transfer\r\n");
    #endif

    while (nbtwi_isBusy()) {
        nbtwi_task();
    }
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
