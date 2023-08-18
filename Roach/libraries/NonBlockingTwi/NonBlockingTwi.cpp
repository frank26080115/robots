#include "NonBlockingTwi.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#define NBTWI_ENABLE_DEBUG

#ifdef NBTWI_ENABLE_DEBUG
extern uint32_t getFreeRam(void);
#endif

//#include "nrfx_twim.h"
//#include "hal/nrf_twim.h"

enum
{
    NBTWI_SM_IDLE,
    NBTWI_SM_STARTED,
    NBTWI_SM_WAIT,
    NBTWI_SM_STOP,
    NBTWI_SM_SUSPEND,
    NBTWI_SM_ERROR,
    NBTWI_SM_TIMEOUT,
};

static uint32_t pin_scl, pin_sda;
static uint8_t* tx_buff = NULL;
static uint32_t tx_buff_sz = 0;

uint32_t nbtwi_delay = 0;

static volatile uint8_t  nbtwi_statemachine = 0;
static volatile bool     nbtwi_isTx = false;
static volatile uint8_t  nbtwi_curFlags = 0;
static volatile bool     xfer_done = true;
static volatile uint32_t xfer_err = 0;
static volatile uint32_t xfer_err_last = 0;
static volatile uint32_t xfer_time = 0;
static volatile uint32_t xfer_time_est = 0;
static volatile uint32_t idle_timestamp = 0;
static volatile uint32_t taskstop_time = 0;
static int err_cnt = 0;

// implement a FIFO with a linked list

typedef struct
{
    void* next;
    uint8_t* data;
    uint8_t i2c_addr;
    int len;
    uint8_t flags;
}
nbtwi_node_t;

enum
{
    NBTWI_FLAG_RWMASK   = 0x03,
    NBTWI_FLAG_WRITE    = 0,
    NBTWI_FLAG_READ     = 1,
    //NBTWI_FLAG_RESULT   = 2,
    NBTWI_FLAG_NOSTOP   = 0x04,
};

static nbtwi_node_t* tx_head = NULL;
static nbtwi_node_t* tx_tail = NULL;
static nbtwi_node_t* rx_head = NULL;
static nbtwi_node_t* rx_tail = NULL;

static uint32_t spd_khz;

static volatile uint32_t* pincfg_reg(uint32_t pin)
{
    NRF_GPIO_Type * port = nrf_gpio_pin_port_decode(&pin);
    return &port->PIN_CNF[pin];
}

void nbtwi_init(int scl, int sda, int bufsz, bool highspeed)
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

    NRF_TWIM0->FREQUENCY = highspeed ? TWIM_FREQUENCY_FREQUENCY_K400 : TWIM_FREQUENCY_FREQUENCY_K100;
    NRF_TWIM0->ENABLE    = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);
    NRF_TWIM0->PSEL.SCL  = g_ADigitalPinMap[pin_scl];
    NRF_TWIM0->PSEL.SDA  = g_ADigitalPinMap[pin_sda];

    spd_khz = highspeed ? 400 : 100;

    if (bufsz == 0) {
        bufsz = 1024;
    }
    tx_buff = (uint8_t*)malloc(bufsz);
    tx_buff_sz = bufsz;
    NRF_TWIM0->TXD.PTR = (uint32_t)(tx_buff);
    NRF_TWIM0->RXD.PTR = (uint32_t)(tx_buff);

    //uint32_t irqn = nrfx_get_irq_number(NRF_TWIM0);
    //NVIC_ClearPendingIRQ(irqn);
    //NVIC_SetPriority(irqn, 3);
    //NVIC_EnableIRQ(irqn);

    #ifdef NBTWI_ENABLE_DEBUG
    Serial.println("NonBlockingTwi begin");
    #endif
}

void nbtwi_write(uint8_t i2c_addr, uint8_t* data, int len, bool no_stop)
{
    nbtwi_node_t* n = (nbtwi_node_t*)malloc(sizeof(nbtwi_node_t));
    if (n == NULL) {
        return;
    }
    n->flags = NBTWI_FLAG_WRITE;
    if (no_stop) {
        n->flags |= NBTWI_FLAG_NOSTOP;
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
    if (tx_tail != NULL)
    {
        tx_tail->next = (void*)n;
        tx_tail = n;
    }
    if (tx_head == NULL) {
        tx_head = n;
        tx_tail = n;
    }
    if (xfer_done) {
        nbtwi_task();
    }
}

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
        #ifdef NBTWI_ENABLE_DEBUG
        Serial.printf(" err no mem node\r\n");
        #endif
        return;
    }
    n->flags = NBTWI_FLAG_WRITE;
    n->next = NULL;
    n->i2c_addr = i2c_addr;
    n->data = (uint8_t*)malloc(len + 1);
    if (n->data == NULL) {
        free(n);
        #ifdef NBTWI_ENABLE_DEBUG
        Serial.printf(" err no mem data\r\n");
        #endif
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

    if (tx_tail != NULL)
    {
        tx_tail->next = (void*)n;
        tx_tail = n;
    }
    if (tx_head == NULL) {
        tx_head = n;
        tx_tail = n;
    }
    #ifdef NBTWI_ENABLE_DEBUG
    Serial.printf(" done, mem %u\r\n", getFreeRam());
    delay(20);
    #endif
    if (xfer_done) {
        nbtwi_task();
    }
}

void nbtwi_read(uint8_t i2c_addr, int len)
{
    nbtwi_node_t* n = (nbtwi_node_t*)malloc(sizeof(nbtwi_node_t));
    if (n == NULL) {
        return;
    }
    n->flags = NBTWI_FLAG_READ;
    n->next = NULL;
    n->i2c_addr = i2c_addr;
    n->data = NULL;
    n->len = len;
    if (tx_tail != NULL)
    {
        tx_tail->next = (void*)n;
        tx_tail = n;
    }
    if (tx_head == NULL) {
        tx_head = n;
        tx_tail = n;
    }
    if (xfer_done) {
        nbtwi_task();
    }
}

bool nbtwi_hasResult(void)
{
    return rx_head != NULL;
}

bool nbtwi_readResult(uint8_t* data, int len, bool clr)
{
    if (rx_head == NULL) {
        return false;
    }

    if (data != NULL) {
        memcpy(data, rx_head->data, len < rx_head->len ? len : rx_head->len);
    }

    if (clr)
    {
        if (rx_tail == rx_head)
        {
            rx_tail = NULL;
        }
        nbtwi_node_t* next = (nbtwi_node_t*)(rx_head->next);
        free(rx_head->data);
        free(rx_head);
        rx_head = next;
    }
    return true;
}

static void nbtwi_handleResult(void)
{
    #ifdef NBTWI_ENABLE_DEBUG
    Serial.printf("nbtwi_handleResult ");
    #endif

    nbtwi_node_t* n = (nbtwi_node_t*)malloc(sizeof(nbtwi_node_t));
    if (n == NULL) {
        return;
    }
    //n->flags = NBTWI_FLAG_RESULT;
    n->next = NULL;
    //n->i2c_addr = NRF_TWIM0->ADDRESS;
    int len = NRF_TWIM0->RXD.MAXCNT;
    n->data = (uint8_t*)malloc(len);
    if (n->data == NULL) {
        free(n);
        #ifdef NBTWI_ENABLE_DEBUG
        Serial.printf("err no mem\r\n");
        #endif
        return;
    }
    memcpy(n->data, tx_buff, len);
    n->len = len;
    #ifdef NBTWI_ENABLE_DEBUG
    int i;
    for (i = 0; i < len; i++)
    {
        Serial.printf("0x%02X ", n->data[i]);
    }
    Serial.printf(">\r\n");
    #endif
    if (rx_tail != NULL)
    {
        rx_tail->next = (void*)n;
        rx_tail = n;
    }
    if (rx_head == NULL) {
        rx_head = n;
        rx_tail = n;
    }
}

static void nbtwi_checkTimeout(void)
{
    uint32_t now = millis();
    if (xfer_time_est != 0 && (now - xfer_time) >= xfer_time_est)
    {
        #ifdef NBTWI_ENABLE_DEBUG
        Serial.printf("nbtwi timeout %u\r\n", xfer_time_est);
        #endif
        nbtwi_statemachine = NBTWI_SM_TIMEOUT;
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
            taskstop_time = millis();
        }
        else
        {
            if (nbtwi_isTx && NRF_TWIM0->EVENTS_TXSTARTED)
            {
                NRF_TWIM0->EVENTS_TXSTARTED = 0;
                nbtwi_statemachine = NBTWI_SM_WAIT;
            }
            else if (nbtwi_isTx == false && NRF_TWIM0->EVENTS_RXSTARTED)
            {
                NRF_TWIM0->EVENTS_RXSTARTED = 0;
                nbtwi_statemachine = NBTWI_SM_WAIT;
            }
            else
            {
                nbtwi_checkTimeout();
            }
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
            taskstop_time = millis();
        }
        else
        {
            if (nbtwi_isTx && NRF_TWIM0->EVENTS_LASTTX)
            {
                NRF_TWIM0->EVENTS_LASTTX = 0;
                if ((nbtwi_curFlags & NBTWI_FLAG_NOSTOP) == 0)
                {
                    NRF_TWIM0->TASKS_STOP = 0x1UL;
                    nbtwi_statemachine = NBTWI_SM_STOP;
                    taskstop_time = millis();
                }
                else
                {
                    NRF_TWIM0->TASKS_SUSPEND = 0x1UL;
                    nbtwi_statemachine = NBTWI_SM_SUSPEND;
                }
            }
            else if (nbtwi_isTx == false && NRF_TWIM0->EVENTS_LASTRX)
            {
                NRF_TWIM0->EVENTS_LASTRX = 0;
                NRF_TWIM0->TASKS_STOP = 0x1UL;
                nbtwi_statemachine = NBTWI_SM_STOP;
                taskstop_time = millis();
                nbtwi_handleResult();
            }
            else
            {
                nbtwi_checkTimeout();
            }
        }
    }
    if (nbtwi_statemachine == NBTWI_SM_TIMEOUT)
    {
        NRF_TWIM0->TASKS_STOP = 0x1UL;
        xfer_err = 0xFF00;
        nbtwi_statemachine = NBTWI_SM_ERROR;
        taskstop_time = millis();
    }
    if (nbtwi_statemachine == NBTWI_SM_SUSPEND)
    {
        if (NRF_TWIM0->EVENTS_SUSPENDED)
        {
            NRF_TWIM0->EVENTS_SUSPENDED = 0;
            nbtwi_statemachine = NBTWI_SM_IDLE;
            xfer_done = true;
            idle_timestamp = millis();
        }
    }
    if (nbtwi_statemachine == NBTWI_SM_STOP || nbtwi_statemachine == NBTWI_SM_ERROR)
    {
        if (NRF_TWIM0->EVENTS_STOPPED)
        {
            NRF_TWIM0->EVENTS_STOPPED = 0;
            xfer_err_last = (nbtwi_statemachine == NBTWI_SM_ERROR) ? xfer_err : 0;
            nbtwi_statemachine = NBTWI_SM_IDLE;
            xfer_done = true;
            idle_timestamp = millis();
        }
        else if ((millis() - taskstop_time) >= 10 && taskstop_time > 0)
        {
            xfer_err_last = (nbtwi_statemachine == NBTWI_SM_ERROR) ? xfer_err : 0;
            nbtwi_statemachine = NBTWI_SM_IDLE;
            xfer_done = true;
            idle_timestamp = millis();
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
        if (nbtwi_delay != 0 && idle_timestamp != 0 && nbtwi_statemachine == NBTWI_SM_IDLE)
        {
            if ((millis() - idle_timestamp) < nbtwi_delay)
            {
                return;
            }
            else
            {
                idle_timestamp = 0;
            }
        }

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

        if (tx_head != NULL)
        {
            if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_WRITE) {
                memcpy(tx_buff, tx_head->data, tx_head->len);
            }

            xfer_done = false;
            xfer_err = 0;

            #ifdef NBTWI_ENABLE_DEBUG
            Serial.printf("nbtwi_task next packet 0x%02X %u", tx_head->i2c_addr, tx_head->len);
            if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_WRITE)
            {
                Serial.printf(" ...");
            }
            else
            {
                Serial.printf("\r\n");
            }
            delay(20);
            #endif

            NRF_TWIM0->ADDRESS        = tx_head->i2c_addr;
            NRF_TWIM0->EVENTS_STOPPED = 0x0UL;
            NRF_TWIM0->TXD.MAXCNT     = (tx_head->len);
            NRF_TWIM0->RXD.MAXCNT     = (tx_head->len);
            NRF_TWIM0->TASKS_RESUME   = 0x1UL;
            nbtwi_curFlags = tx_head->flags;
            if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_WRITE)
            {
                NRF_TWIM0->TASKS_STARTTX  = 0x1UL;
                nbtwi_isTx = true;
            }
            else if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_READ)
            {
                NRF_TWIM0->TASKS_STARTRX  = 0x1UL;
                nbtwi_isTx = false;
            }
            xfer_time = millis();
            xfer_time_est = ((tx_head->len + 1) * 9 * 4) / spd_khz;
            xfer_time_est = (xfer_time_est < 5) ? 5 : xfer_time_est;
            nbtwi_statemachine = NBTWI_SM_STARTED;

            #ifdef NBTWI_ENABLE_DEBUG
            if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_WRITE)
            {
                int i;
                for (i = 0; i < (tx_head->len); i++) {
                    Serial.printf(" 0x%02X", tx_buff[i]);
                }
                Serial.printf(" go\r\n");
            }
            #endif

            if (tx_tail == tx_head)
            {
                tx_tail = NULL;
            }
            nbtwi_node_t* next = (nbtwi_node_t*)(tx_head->next);
            free(tx_head->data);
            free(tx_head);
            tx_head = next;
        }
    }
}

bool nbtwi_isBusy(void)
{
    return xfer_done == false || tx_head != NULL;
}

void nbtwi_transfer(void)
{
    while (nbtwi_isBusy()) {
        nbtwi_task();
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

int nbtwi_lastError(void)
{
    return xfer_err_last;
}

uint8_t nbtwi_scan(uint8_t start)
{
    uint16_t i;
    for (i = start; i <= 0xFF; i++)
    {
        Serial.printf("scan 0x%02X\r\n", i);
        xfer_err = 0;
        err_cnt = 0;
        nbtwi_write((uint8_t)i, (uint8_t*)&i, 1, false);
        nbtwi_transfer();
        if (err_cnt == 0 && xfer_err == 0) {
            return i;
        }
    }
    return 0;
}
