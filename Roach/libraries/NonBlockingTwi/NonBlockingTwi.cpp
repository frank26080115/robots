#include "NonBlockingTwi.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

//#define NBTWI_ENABLE_DEBUG

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
    NBTWI_SM_RESUMED,
    NBTWI_SM_ERROR,
    NBTWI_SM_TIMEOUT,
    NBTWI_SM_FORCESTOP,
    NBTWI_SM_FORCESTOP_WAIT,
    NBTWI_SM_FORCESTOP_WAIT2,
};

static uint32_t pin_scl, pin_sda, freq, pin_params;
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
static volatile bool     resume_required = false;
static volatile int      start_retry = 0;
static volatile uint32_t idle_timestamp = 0;
static volatile uint32_t taskstop_time = 0;
static volatile uint16_t timeout_location;
static volatile uint16_t timeout_flags;
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
    NBTWI_FLAG_RWMASK   = 0x01,
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

#ifdef __cplusplus
extern "C" {
#endif
extern nrfx_err_t nrfx_twi_twim_bus_recover(uint32_t scl_pin, uint32_t sda_pin);
// note: this function does not help
#ifdef __cplusplus
}
#endif

void nbtwi_init(int scl, int sda, int bufsz, bool highspeed)
{
    *pincfg_reg(g_ADigitalPinMap[pin_scl = scl]) = pin_params =
                                                   ((uint32_t)GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_PULL_Pullup      << GPIO_PIN_CNF_PULL_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0D1       << GPIO_PIN_CNF_DRIVE_Pos)
                                                 | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);

    *pincfg_reg(g_ADigitalPinMap[pin_sda = sda]) = pin_params;

    NRF_TWIM0->FREQUENCY = freq = (highspeed ? TWIM_FREQUENCY_FREQUENCY_K400 : TWIM_FREQUENCY_FREQUENCY_K100);
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

void nbtwi_reinit(void)
{
    *pincfg_reg(g_ADigitalPinMap[pin_scl]) = pin_params;
    *pincfg_reg(g_ADigitalPinMap[pin_sda]) = pin_params;
    NRF_TWIM0->FREQUENCY = freq;
    NRF_TWIM0->PSEL.SCL  = g_ADigitalPinMap[pin_scl];
    NRF_TWIM0->PSEL.SDA  = g_ADigitalPinMap[pin_sda];
    NRF_TWIM0->TXD.PTR   = (uint32_t)(tx_buff);
    NRF_TWIM0->RXD.PTR   = (uint32_t)(tx_buff);
    NRF_TWIM0->ENABLE    = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);
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

static void nbtwi_checkTimeout(uint16_t location)
{
    uint32_t now = millis();
    if (xfer_time_est != 0 && (now - xfer_time) >= xfer_time_est)
    {
        #ifdef NBTWI_ENABLE_DEBUG
        Serial.printf("nbtwi timeout %u\r\n", xfer_time_est);
        #endif
        nbtwi_statemachine = NBTWI_SM_TIMEOUT;

        // track where the timeout happened and what state the TWIM is in
        // for debugging purposes
        timeout_location = location;
        if (nbtwi_isTx) {
            timeout_location |= 0x8000;
        }
        timeout_flags = 0
            | (((NRF_TWIM0->EVENTS_STOPPED   != 0) ? 1 : 0) << 0)
            | (((NRF_TWIM0->EVENTS_ERROR     != 0) ? 1 : 0) << 1)
            | (((NRF_TWIM0->EVENTS_SUSPENDED != 0) ? 1 : 0) << 2)
            | (((NRF_TWIM0->EVENTS_RXSTARTED != 0) ? 1 : 0) << 3)
            | (((NRF_TWIM0->EVENTS_TXSTARTED != 0) ? 1 : 0) << 4)
            | (((NRF_TWIM0->EVENTS_LASTRX    != 0) ? 1 : 0) << 5)
            | (((NRF_TWIM0->EVENTS_LASTTX    != 0) ? 1 : 0) << 6)
            | (((NRF_TWIM0->RXD.AMOUNT >= NRF_TWIM0->RXD.MAXCNT) ? 1 : 0) << 7)
        ;

        #if 1
        if (timeout_flags == 0x80)
        {
            nbtwi_statemachine = NBTWI_SM_FORCESTOP;
        }
        #else
        nrfx_twi_twim_bus_recover(g_ADigitalPinMap[pin_scl], g_ADigitalPinMap[pin_sda]);
        #endif
    }
}

static void nbtwi_runStateMachine(void)
{
    if (nbtwi_statemachine == NBTWI_SM_FORCESTOP)
    {
        NRF_TWIM0->TASKS_STOP = 0x1UL;
        taskstop_time = millis();
        nbtwi_statemachine = NBTWI_SM_FORCESTOP_WAIT;
        return;
    }
    else if (nbtwi_statemachine == NBTWI_SM_FORCESTOP_WAIT)
    {
        if (NRF_TWIM0->EVENTS_STOPPED)
        {
            NRF_TWIM0->EVENTS_STOPPED = 0;
            nbtwi_statemachine = NBTWI_SM_FORCESTOP_WAIT2;
            
        }
        else if ((millis() - taskstop_time) >= 1 && taskstop_time > 0)
        {
            nbtwi_statemachine = NBTWI_SM_FORCESTOP_WAIT2;
            xfer_done = true;
            idle_timestamp = millis();
        }
        if (nbtwi_statemachine != NBTWI_SM_FORCESTOP_WAIT)
        {
            NRF_TWIM0->EVENTS_STOPPED   = 0;
            NRF_TWIM0->EVENTS_ERROR     = 0;
            NRF_TWIM0->EVENTS_TXSTARTED = 0;
            NRF_TWIM0->EVENTS_RXSTARTED = 0;
            NRF_TWIM0->EVENTS_LASTTX    = 0;
            NRF_TWIM0->EVENTS_LASTRX    = 0;
            NRF_TWIM0->EVENTS_SUSPENDED = 0;
            NRF_TWIM0->PSEL.SCL         = 0xFFFFFFFF;
            NRF_TWIM0->PSEL.SDA         = 0xFFFFFFFF;
            NRF_TWIM0->ENABLE           = 0;
            pinMode(pin_sda, OUTPUT);
            pinMode(pin_scl, OUTPUT);
            digitalWrite(pin_sda, HIGH);
            digitalWrite(pin_scl, HIGH);
            taskstop_time = millis();
        }
        return;
    }
    else if (nbtwi_statemachine == NBTWI_SM_FORCESTOP_WAIT2)
    {
        if ((millis() - taskstop_time) >= 1)
        {
            nbtwi_reinit();
            xfer_done = true;
            idle_timestamp = millis();
            taskstop_time  = idle_timestamp;
            xfer_err_last  = xfer_err = 0xF000;
            nbtwi_statemachine = NBTWI_SM_IDLE;
        }
        return;
    }

    if (nbtwi_statemachine == NBTWI_SM_STARTED || nbtwi_statemachine == NBTWI_SM_RESUMED)
    {
        if (NRF_TWIM0->EVENTS_ERROR)
        {
            NRF_TWIM0->EVENTS_ERROR = 0;
            xfer_err = NRF_TWIM0->ERRORSRC;
            NRF_TWIM0->ERRORSRC = 0;
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
            else if (nbtwi_statemachine == NBTWI_SM_RESUMED)
            {
                NRF_TWIM0->TASKS_RESUME = 1UL;
                nbtwi_statemachine = NBTWI_SM_STARTED;
                resume_required = false;
            }
            #if 0
            else if (start_retry < 2 && xfer_time_est != 0 && (millis() - xfer_time) >= xfer_time_est)
            {
                NRF_TWIM0->TASKS_RESUME = 1UL;
                if (nbtwi_isTx) {
                    NRF_TWIM0->TASKS_STARTTX = 1UL;
                }
                else {
                    NRF_TWIM0->TASKS_STARTRX = 1UL;
                }
                start_retry++;
                xfer_time = millis();
            }
            #endif
            else
            {
                nbtwi_checkTimeout(__LINE__);
            }
        }
    }
    if (nbtwi_statemachine == NBTWI_SM_WAIT)
    {
        if (NRF_TWIM0->EVENTS_ERROR)
        {
            NRF_TWIM0->EVENTS_ERROR = 0;
            xfer_err = NRF_TWIM0->ERRORSRC;
            NRF_TWIM0->ERRORSRC = 0;
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
            else if (nbtwi_isTx == false && NRF_TWIM0->EVENTS_LASTRX != 0)
            {
                NRF_TWIM0->EVENTS_LASTRX = 0;
                NRF_TWIM0->TASKS_STOP = 0x1UL;
                nbtwi_statemachine = NBTWI_SM_STOP;
                taskstop_time = millis();
                nbtwi_handleResult();
            }
            else
            {
                nbtwi_checkTimeout(__LINE__);
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
            resume_required = true;
            xfer_done = true;
            idle_timestamp = millis();
        }
        else
        {
            nbtwi_checkTimeout(__LINE__);
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
        else if ((millis() - taskstop_time) >= 50 && taskstop_time > 0)
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
            xfer_time = millis();
            xfer_time_est = ((tx_head->len + 1) * 9 * 6) / spd_khz;
            xfer_time_est = (xfer_time_est < 5) ? 5 : xfer_time_est;
            if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_WRITE)
            {
                NRF_TWIM0->TASKS_STARTTX  = 0x1UL;
                nbtwi_isTx = true;
                nbtwi_statemachine = NBTWI_SM_STARTED;
            }
            else if ((tx_head->flags & NBTWI_FLAG_RWMASK) == NBTWI_FLAG_READ)
            {
                NRF_TWIM0->TASKS_STARTRX  = 0x1UL;
                nbtwi_isTx = false;
                nbtwi_statemachine = resume_required ? NBTWI_SM_RESUMED : NBTWI_SM_STARTED;
            }
            else
            {
                Serial.printf("ERROR [%u] NBTWI did not actually start transaction\r\n", millis());
            }

            resume_required = false;
            start_retry = 0;

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

            if (nbtwi_statemachine == NBTWI_SM_STARTED)
            {
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

void nbtwi_forceStop(void)
{
    nbtwi_statemachine = NBTWI_SM_FORCESTOP;
    xfer_done = false;
    while (tx_head != NULL)
    {
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

uint16_t nbtwi_getTimeoutLocation(void)
{
    return timeout_location;
}

uint16_t nbtwi_getTimeoutFlags(void)
{
    return timeout_flags;
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
