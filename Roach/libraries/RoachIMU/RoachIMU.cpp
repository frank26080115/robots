#include "RoachIMU.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static RoachIMU* instance = NULL;

static int  i2chal_write (sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);
static int  i2chal_read  (sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
static void i2chal_close (sh2_Hal_t *self);
static int  i2chal_open  (sh2_Hal_t *self);
static uint32_t hal_getTimeUs(sh2_Hal_t *self);
static void hal_callback(void *cookie, sh2_AsyncEvent_t *pEvent);
static void sensorHandler(void *cookie, sh2_SensorEvent_t *pEvent);
static void hal_hardwareReset(void);

#if defined(NRF52840_XXAA)
static void twi_handler(nrfx_twim_evt_t const * p_event, void * p_context);
static nrfx_twim_t m_twi = {.p_twim = NRF_TWIM0, .drv_inst_idx = 0, };//NRFX_TWIM_INSTANCE(TWI_INSTANCE_ID);
static volatile bool m_xfer_done = false;
static volatile int  m_xfer_err = 0;

#ifdef __cplusplus
extern "C" {
#endif

extern nrfx_err_t nrfx_twim_init(nrfx_twim_t const *        p_instance,
                                 nrfx_twim_config_t const * p_config,
                                 nrfx_twim_evt_handler_t    event_handler,
                                 void *                     p_context);

extern nrfx_err_t nrfx_twim_tx(nrfx_twim_t const * p_instance,
                               uint8_t             address,
                               uint8_t     const * p_data,
                               size_t              length,
                               bool                no_stop);

extern nrfx_err_t nrfx_twim_rx(nrfx_twim_t const * p_instance,
                               uint8_t             address,
                               uint8_t *           p_data,
                               size_t              length);

#ifdef __cplusplus
}
#endif

#endif

static void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees);
static void quaternionToEulerRV(sh2_RotationVectorWAcc_t* rotational_vector, euler_t* ypr, bool degrees);
static void quaternionToEulerGI(sh2_GyroIntegratedRV_t* rotational_vector, euler_t* ypr, bool degrees);

RoachIMU::RoachIMU(int samp_interval, int rd_interval, int orientation, int dev_addr, int rst, int sda, int scl)
{
    sample_interval = samp_interval;
    read_interval = rd_interval;
    install_orientation = orientation;
    i2c_addr = dev_addr;
    pin_rst = rst;
    pin_sda = sda;
    pin_scl = scl;
    instance = this;
}

void RoachIMU::begin(void)
{
    #if defined(ESP32)
    nbe_i2c_init(&nbe_i2c, 0, (gpio_num_t)pin_sda, (gpio_num_t)pin_scl, 400000);
    #elif defined(NRF52840_XXAA)
    nrfx_twim_config_t twi_config = {
       .scl                = (uint32_t)pin_scl,
       .sda                = (uint32_t)pin_sda,
       .frequency          = NRF_TWIM_FREQ_400K,
       .interrupt_priority = 2,
    };
    nrfx_twim_init(&m_twi, &twi_config, twi_handler, NULL);
    #endif
    hal.open = i2chal_open;
    hal.close = i2chal_close;
    hal.read = i2chal_read;
    hal.write = i2chal_write;
    hal.getTimeUs = hal_getTimeUs;
    state_machine = ROACHIMU_SM_SETUP;
}

void RoachIMU::task(void)
{
    if (reset_occured) {
        state_machine = ROACHIMU_SM_SETUP;
    }

    switch (state_machine)
    {
        case ROACHIMU_SM_SETUP:
            {
                #if defined(ESP32)
                if (nbe_i2c_is_busy(&nbe_i2c)) {
                    state_machine = ROACHIMU_SM_ERROR_WAIT;
                    error_time = millis();
                    break;
                }
                #elif defined(NRF52840_XXAA)
                #endif
                reset_occured = false;
                int err_ret;
                hal_hardwareReset();
                err_ret = sh2_open(&hal, hal_callback, NULL);
                if (err_ret != SH2_OK) {
                    state_machine = ROACHIMU_SM_ERROR_WAIT;
                    error_time = millis();
                    break;
                }
                memset(&prodIds, 0, sizeof(prodIds));
                err_ret = sh2_getProdIds(&prodIds);
                if (err_ret != SH2_OK) {
                    state_machine = ROACHIMU_SM_ERROR_WAIT;
                    error_time = millis();
                    break;
                }
                sh2_setSensorCallback(sensorHandler, NULL);
                static sh2_SensorConfig_t config;
                // These sensor options are disabled or not used in most cases
                config.changeSensitivityEnabled = false;
                config.wakeupEnabled = false;
                config.changeSensitivityRelative = false;
                config.alwaysOnEnabled = false;
                config.changeSensitivity = 0;
                config.batchInterval_us = 0;
                config.sensorSpecific = 0;
                config.reportInterval_us = sample_interval;
                err_ret = sh2_setSensorConfig(SH2_GYRO_INTEGRATED_RV, &config);
                if (err_ret != SH2_OK) {
                    state_machine = ROACHIMU_SM_ERROR_WAIT;
                    error_time = millis();
                    break;
                }
                state_machine = ROACHIMU_SM_SVC_START;
            }
            break;
        case ROACHIMU_SM_SVC_START:
        case ROACHIMU_SM_SVC_GET_HEADER:
            {
                int len = 4;
                #if defined(ESP32)
                nbe_i2c_reset(&nbe_i2c);
                tx_buff[0] = i2c_first_byte_read(i2c_addr);
                nbe_i2c_set_tx_buf(&nbe_i2c, tx_buff);
                nbe_i2c_set_rx_buf(&nbe_i2c, rx_buff_i2c);
                nbe_i2c_start(&nbe_i2c);
                if (len > 1) {
                    nbe_i2c_read_ack(&nbe_i2c, len - 1);
                }
                nbe_i2c_read_nak(&nbe_i2c, 1);
                nbe_i2c_commit(&nbe_i2c);
                #elif defined(NRF52840_XXAA)
                m_xfer_done = false;
                m_xfer_err = 0;
                nrfx_err_t sts = nrfx_twim_rx(&m_twi, i2c_addr, (uint8_t*)&rx_buff_i2c, len);
                if (sts != NRF_SUCCESS) {
                    state_machine = ROACHIMU_SM_ERROR;
                    break;
                }
                #endif
                state_machine = ROACHIMU_SM_SVC_GET_HEADER_WAIT;
                read_time = millis();
            }
            break;
        case ROACHIMU_SM_SVC_GET_HEADER_WAIT:
            {
                bool timeout = (millis() - read_time) >= 100;
                if (
                    #if defined(ESP32)
                        nbe_i2c_is_busy(&nbe_i2c) == false
                    #elif defined(NRF52840_XXAA)
                        m_xfer_done || timeout
                    #endif
                    )
                {
                    if (
                        #if defined(ESP32)
                            nbe_i2c.error != 0
                        #elif defined(NRF52840_XXAA)
                            m_xfer_err != 0 || timeout
                        #endif
                        )
                    {
                        state_machine = ROACHIMU_SM_ERROR;
                        break;
                    }
                    #if defined(NRF52840_XXAA)
                    m_xfer_done = false;
                    m_xfer_err = 0;
                    #endif

                    uint8_t* header = (uint8_t*)rx_buff_i2c;
                    
                    // Determine amount to read
                    uint16_t packet_size = (uint16_t)header[0] | (uint16_t)header[1] << 8;
                    // Unset the "continue" bit
                    packet_size &= ~0x8000;
                    if (packet_size > SH2_HAL_MAX_TRANSFER_IN) {
                        // this should never happen
                        Serial.printf("ERR[%u]: IMU packet size too big %d\r\n", millis(), packet_size);
                        state_machine = ROACHIMU_SM_ERROR;
                        break;
                    }

                    // if packet size is zero, it will trigger the done-condition on the next state

                    svc_reader.cargo_remaining = packet_size;
                    svc_reader.cargo_read_amount = 0;
                    svc_reader.first_read = true;
                    rx_buff_wptr = 0;
                    state_machine = ROACHIMU_SM_SVC_READ_CHUNK;
                }
            }
            break;
        case ROACHIMU_SM_SVC_READ_CHUNK:
            {
                if (svc_reader.cargo_remaining <= 0)
                {
                    state_machine = ROACHIMU_SM_SVC_READ_DONE;
                    break;
                }
                if (svc_reader.first_read) {
                    svc_reader.read_size = min(ROACHIMU_BUFF_RX_SIZE, svc_reader.cargo_remaining);
                } else {
                    svc_reader.read_size = min(ROACHIMU_BUFF_RX_SIZE, svc_reader.cargo_remaining + 4);
                }
                int len = svc_reader.read_size;

                #if defined(ESP32)
                nbe_i2c_reset(&nbe_i2c);
                tx_buff[0] = i2c_first_byte_read(i2c_addr);
                nbe_i2c_set_tx_buf(&nbe_i2c, tx_buff);
                nbe_i2c_set_rx_buf(&nbe_i2c, rx_buff_i2c);
                nbe_i2c_start(&nbe_i2c);
                if (len > 1) {
                    nbe_i2c_read_ack(&nbe_i2c, len - 1);
                }
                nbe_i2c_read_nak(&nbe_i2c, 1);
                nbe_i2c_commit(&nbe_i2c);
                #elif defined(NRF52840_XXAA)
                m_xfer_done = false;
                m_xfer_err = 0;
                nrfx_err_t sts = nrfx_twim_rx(&m_twi, i2c_addr, (uint8_t*)&rx_buff_i2c, len);
                if (sts != NRF_SUCCESS) {
                    state_machine = ROACHIMU_SM_ERROR;
                    break;
                }
                #endif

                state_machine = ROACHIMU_SM_SVC_READ_CHUNK_WAIT;
            }
            break;
        case ROACHIMU_SM_SVC_READ_CHUNK_WAIT:
            {
                bool timeout = (millis() - read_time) >= 300;
                if (
                    #if defined(ESP32)
                        nbe_i2c_is_busy(&nbe_i2c) == false
                    #elif defined(NRF52840_XXAA)
                        m_xfer_done || timeout
                    #endif
                    )
                {
                    if (
                        #if defined(ESP32)
                            nbe_i2c.error != 0
                        #elif defined(NRF52840_XXAA)
                            m_xfer_err != 0 || timeout
                        #endif
                        )
                    {
                        state_machine = ROACHIMU_SM_ERROR;
                        break;
                    }
                    #if defined(NRF52840_XXAA)
                    m_xfer_done = false;
                    m_xfer_err = 0;
                    #endif

                    if (svc_reader.first_read)
                    {
                        rx_buff_wptr = 0;
                        // The first time we're saving the "original" header, so include it in the cargo count
                        svc_reader.cargo_read_amount = svc_reader.read_size;
                        memcpy(&rx_buff_sh2[rx_buff_wptr], rx_buff_i2c, svc_reader.cargo_read_amount);
                        svc_reader.first_read = false;
                    } else
                    {
                        // this is not the first read, so copy from 4 bytes after the beginning of
                        // the i2c buffer to skip the header included with every new i2c read and
                        // don't include the header in the amount of cargo read
                        svc_reader.cargo_read_amount = svc_reader.read_size - 4;
                        memcpy(&rx_buff_sh2[rx_buff_wptr], &rx_buff_i2c[4], svc_reader.cargo_read_amount);
                    }
                    // advance our pointer by the amount of cargo read
                    rx_buff_wptr += svc_reader.cargo_read_amount;
                    // mark the cargo as received
                    svc_reader.cargo_remaining -= svc_reader.cargo_read_amount;
                    state_machine = ROACHIMU_SM_SVC_READ_CHUNK;
                }
            }
            break;
        case ROACHIMU_SM_SVC_READ_DONE:
            {
                if (svc_reader.cargo_remaining <= 0 && rx_buff_wptr > 0) // have data
                {
                    sh2_rxAssemble(sh2_getShtpInstance(), rx_buff_sh2, rx_buff_wptr, read_time * 1000); // this will call whatever callback was assigned
                    rx_buff_wptr = 0;
                    sample_time = read_time;
                }
                state_machine = ROACHIMU_SM_SVC_READ_DONE_LOOP;
            }
            break;
        case ROACHIMU_SM_SVC_READ_DONE_LOOP:
            state_machine = ROACHIMU_SM_SVC_READ_DONE_WAIT;
            break;
        case ROACHIMU_SM_SVC_READ_DONE_WAIT:
            if ((millis() - read_time) >= read_interval) { // read_interval can be zero for no-waiting
                state_machine = ROACHIMU_SM_SVC_GET_HEADER;
            }
            break;
        case ROACHIMU_SM_ERROR:
            state_machine = ROACHIMU_SM_SETUP;
            is_ready = false;
            break;
        case ROACHIMU_SM_ERROR_WAIT:
            {
                is_ready = false;
                if ((millis() - error_time) >= 500) {
                    state_machine = ROACHIMU_SM_SETUP;
                }
            }
            break;
    }
}

void RoachIMU::pause_service(void)
{
    // finish up any on-going transactions and leave it in the done state
    while (state_machine >= ROACHIMU_SM_SVC_START && state_machine < ROACHIMU_SM_SVC_READ_DONE_LOOP) {
        task();
        yield();
    }
}

void RoachIMU::tare(void)
{
    pause_service();
    sh2_setTareNow(0x07, (sh2_TareBasis_t)0x05); // tare all three axis, for gyro-integrated-RV
}

void RoachIMU::do_math(void)
{
    if (has_new)
    {
        euler_t eu;
        memcpy((void*)&(girv), (void*)&(sensor_value.un.gyroIntegratedRV), sizeof(sh2_GyroIntegratedRV_t));
        quaternionToEulerGI(&(girv), &(eu), true);
        has_new = false;
        // reorder the euler angles according to installation orientation
        uint8_t ori = install_orientation & 0x0F;
        switch (ori)
        {
            case ROACHIMU_ORIENTATION_XYZ:
                memcpy((void*)&(eu), (void*)&(euler), sizeof(euler_t));
                break;
            case ROACHIMU_ORIENTATION_XZY:
                euler.pitch = eu.yaw;
                euler.roll  = eu.roll;
                euler.yaw   = eu.pitch;
                break;
            case ROACHIMU_ORIENTATION_YXZ:
                euler.roll  = eu.pitch;
                euler.pitch = eu.roll;
                euler.yaw   = eu.yaw;
                break;
            case ROACHIMU_ORIENTATION_YZX:
                euler.roll  = eu.pitch;
                euler.pitch = eu.yaw;
                euler.yaw   = eu.roll;
                break;
            case ROACHIMU_ORIENTATION_ZXY:
                euler.roll  = eu.yaw;
                euler.pitch = eu.roll;
                euler.yaw   = eu.pitch;
                break;
            case ROACHIMU_ORIENTATION_ZYX:
                euler.roll  = eu.yaw;
                euler.pitch = eu.pitch;
                euler.yaw   = eu.roll;
                break;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_FLIP_ROLL) != 0) {
            euler.roll *= -1;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_FLIP_PITCH) != 0) {
            euler.pitch *= -1;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_FLIP_YAW) != 0) {
            euler.yaw *= -1;
        }
        float invert_hysterisis = 2;
        invert_hysterisis *= is_inverted ? -1 : 1;
        is_inverted = (euler.roll > (90 + invert_hysterisis) || euler.roll < (-90 - invert_hysterisis)) || (euler.pitch > (90 + invert_hysterisis) || euler.pitch < (-90 - invert_hysterisis));
        heading = euler.yaw * (is_inverted ? -1 : 1);
    }
}

#if defined(NRF52840_XXAA)
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
#endif

bool RoachIMU::i2c_write(uint8_t* buf, int len)
{
    pause_service();

    #if defined(ESP32)
    nbe_i2c_reset(&nbe_i2c);
    int i;
    tx_buff[0] = i2c_first_byte_write(i2c_addr);
    for (i = 0; i < len; i++)
    {
        tx_buff[i + 1] = buf[i];
    }
    nbe_i2c_set_tx_buf(&nbe_i2c, tx_buff);
    nbe_i2c_set_rx_buf(&nbe_i2c, rx_buff_i2c);
    nbe_i2c_start(&nbe_i2c);
    nbe_i2c_write(&nbe_i2c, 1 + len);
    nbe_i2c_stop(&nbe_i2c);
    nbe_i2c_commit(&nbe_i2c);
    while (nbe_i2c_is_busy(&nbe_i2c)) {
        yield(); // or whatever you want to do
    }
    return nbe_i2c.error == 0;
    #elif defined(NRF52840_XXAA)
    uint32_t t = millis();
    nrfx_err_t sts = nrfx_twim_tx(&m_twi, i2c_addr, buf, len, false);
    if (sts != NRF_SUCCESS) {
        return false;
    }
    while (m_xfer_done == false && m_xfer_err == 0) {
        yield();
        if ((millis() - t) >= 100) {
            break;
        }
    }
    return m_xfer_done && m_xfer_err == 0;
    #endif
}

bool RoachIMU::i2c_read(uint8_t* buf, int len)
{
    pause_service();

    int i;

    #if defined(ESP32)
    nbe_i2c_reset(&nbe_i2c);
    tx_buff[0] = i2c_first_byte_read(i2c_addr);
    nbe_i2c_set_tx_buf(&nbe_i2c, tx_buff);
    nbe_i2c_set_rx_buf(&nbe_i2c, rx_buff_i2c);
    nbe_i2c_start(&nbe_i2c);
    if (len > 1) {
        nbe_i2c_read_ack(&nbe_i2c, len - 1);
    }
    nbe_i2c_read_nak(&nbe_i2c, 1);
    nbe_i2c_commit(&nbe_i2c);
    while (nbe_i2c_is_busy(&nbe_i2c)) {
        yield(); // or whatever you want to do
    }
    if (nbe_i2c.error != 0) {
        return false;
    }
    #elif defined(NRF52840_XXAA)
    uint32_t t = millis();
    nrfx_err_t sts = nrfx_twim_rx(&m_twi, i2c_addr, rx_buff_i2c, len);
    if (sts != NRF_SUCCESS) {
        return false;
    }
    while (m_xfer_done == false && m_xfer_err == 0) {
        yield();
        if ((millis() - t) >= 100) {
            break;
        }
    }
    if (m_xfer_done == false || m_xfer_err != 0) {
        return false;
    }
    #endif
    for (i = 0; i < len; i++)
    {
        buf[i] = rx_buff_i2c[i];
    }
    return true;
}

static int i2chal_open(sh2_Hal_t *self)
{
    uint8_t softreset_pkt[] = {5, 0, 1, 0, 1};
    bool success = false;
    for (uint8_t attempts = 0; attempts < 5; attempts++) {
        if (instance->i2c_write(softreset_pkt, 5)) {
            success = true;
            break;
        }
        delay(30);
    }
    if (!success)
        return -1;
    delay(300);
    return 0;
}

static void i2chal_close(sh2_Hal_t *self)
{
}

static int i2chal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us)
{
    // uint8_t *pBufferOrig = pBuffer;

    uint8_t header[4];
    if (!instance->i2c_read(header, 4)) {
        return 0;
    }

    // Determine amount to read
    uint16_t packet_size = (uint16_t)header[0] | (uint16_t)header[1] << 8;
    // Unset the "continue" bit
    packet_size &= ~0x8000;

    size_t i2c_buffer_max = ROACHIMU_BUFF_RX_SIZE;

    if (packet_size > len) {
        // packet wouldn't fit in our buffer
        return 0;
    }
    // the number of non-header bytes to read
    uint16_t cargo_remaining = packet_size;
    uint8_t i2c_buffer[i2c_buffer_max];
    uint16_t read_size;
    uint16_t cargo_read_amount = 0;
    bool first_read = true;

    while (cargo_remaining > 0) {
        if (first_read) {
            read_size = min(i2c_buffer_max, (size_t)cargo_remaining);
        } else {
            read_size = min(i2c_buffer_max, (size_t)cargo_remaining + 4);
        }

        // Serial.print("Reading from I2C: ");  Serial.println(read_size);
        // Serial.print("Remaining to read: "); Serial.println(cargo_remaining);

        if (!instance->i2c_read(i2c_buffer, read_size)) {
            return 0;
        }

        if (first_read) {
            // The first time we're saving the "original" header, so include it in the
            // cargo count
            cargo_read_amount = read_size;
            memcpy(pBuffer, i2c_buffer, cargo_read_amount);
            first_read = false;
        } else {
            // this is not the first read, so copy from 4 bytes after the beginning of
            // the i2c buffer to skip the header included with every new i2c read and
            // don't include the header in the amount of cargo read
            cargo_read_amount = read_size - 4;
            memcpy(pBuffer, i2c_buffer + 4, cargo_read_amount);
        }
        // advance our pointer by the amount of cargo read
        pBuffer += cargo_read_amount;
        // mark the cargo as received
        cargo_remaining -= cargo_read_amount;
    }

    return packet_size;
}

static int i2chal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    size_t i2c_buffer_max = ROACHIMU_BUFF_TX_SIZE;

    uint16_t write_size = min(i2c_buffer_max, len);
    if (!instance->i2c_write(pBuffer, write_size)) {
        return 0;
    }

    return write_size;
}

static void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees = false)
{

    float sqr = sq(qr);
    float sqi = sq(qi);
    float sqj = sq(qj);
    float sqk = sq(qk);

    ypr->yaw   = atan2( 2.0 * (qi * qj + qk * qr) , ( sqi - sqj - sqk + sqr));
    ypr->pitch = asin (-2.0 * (qi * qk - qj * qr) / ( sqi + sqj + sqk + sqr));
    ypr->roll  = atan2( 2.0 * (qj * qk + qi * qr) , (-sqi - sqj + sqk + sqr));

    if (degrees) {
        ypr->yaw   *= RAD_TO_DEG;
        ypr->pitch *= RAD_TO_DEG;
        ypr->roll  *= RAD_TO_DEG;
    }
}

static void quaternionToEulerRV(sh2_RotationVectorWAcc_t* rotational_vector, euler_t* ypr, bool degrees = false)
{
    quaternionToEuler(rotational_vector->real, rotational_vector->i, rotational_vector->j, rotational_vector->k, ypr, degrees);
}

static void quaternionToEulerGI(sh2_GyroIntegratedRV_t* rotational_vector, euler_t* ypr, bool degrees = false)
{
    quaternionToEuler(rotational_vector->real, rotational_vector->i, rotational_vector->j, rotational_vector->k, ypr, degrees);
}

static void hal_hardwareReset(void)
{
    if (instance->pin_rst != -1)
    {
        pinMode(instance->pin_rst, OUTPUT);
        digitalWrite(instance->pin_rst, HIGH);
        delay(10);
        digitalWrite(instance->pin_rst, LOW);
        delay(10);
        digitalWrite(instance->pin_rst, HIGH);
        delay(10);
    }
}

static uint32_t hal_getTimeUs(sh2_Hal_t *self)
{
    return millis() * 1000;
}

static void hal_callback(void *cookie, sh2_AsyncEvent_t *pEvent)
{
    // If we see a reset, set a flag so that sensors will be reconfigured.
    if (pEvent->eventId == SH2_RESET) {
        instance->reset_occured = true;
    }
}

static void sensorHandler(void *cookie, sh2_SensorEvent_t *event)
{
    int rc;
    rc = sh2_decodeSensorEvent(&(instance->sensor_value), event);

    if (rc != SH2_OK) {
        instance->sensor_value.timestamp = 0;
        return;
    }

    if (instance->sensor_value.sensorId == SH2_GYRO_INTEGRATED_RV)
    {
        instance->is_ready = true;
        instance->has_new = true;
    }
}
