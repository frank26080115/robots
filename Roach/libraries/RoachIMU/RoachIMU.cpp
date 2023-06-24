#include "RoachIMU.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ROACHIMU_ENABLE_DEBUG

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

#endif

static void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees);
static void quaternionToEulerRV(sh2_RotationVectorWAcc_t* rotational_vector, euler_t* ypr, bool degrees);
static void quaternionToEulerGI(sh2_GyroIntegratedRV_t* rotational_vector, euler_t* ypr, bool degrees);

RoachIMU::RoachIMU(int samp_interval, int rd_interval, int orientation, int dev_addr, int rst)
{
    sample_interval = samp_interval;
    read_interval = rd_interval;
    install_orientation = orientation;
    i2c_addr = dev_addr;
    pin_rst = rst;
    instance = this;
}

void RoachIMU::begin(void)
{
    hal.open = i2chal_open;
    hal.close = i2chal_close;
    hal.read = i2chal_read;
    hal.write = i2chal_write;
    hal.getTimeUs = hal_getTimeUs;
    state_machine = ROACHIMU_SM_SETUP;
}

void RoachIMU::task(void)
{
    nbtwi_task();

    if (reset_occured && state_machine >= ROACHIMU_SM_SVC_GET_HEADER) {
        Serial.printf("IMU reset_occured %u\r\n", millis());
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
                int err_ret;
                hal_hardwareReset();
                #ifdef ROACHIMU_ENABLE_DEBUG
                Serial.printf("IMU init calling sh2_open...\r\n");
                #endif
                err_ret = sh2_open(&hal, hal_callback, NULL);
                if (err_ret != SH2_OK) {
                    Serial.printf("IMU init err sh2_open 0x%02X\r\n", err_ret);
                    state_machine = ROACHIMU_SM_ERROR_WAIT;
                    error_time = millis();
                    break;
                }
                memset(&prodIds, 0, sizeof(prodIds));
                err_ret = sh2_getProdIds(&prodIds);
                if (err_ret != SH2_OK) {
                    Serial.printf("IMU init err sh2_getProdIds 0x%02X\r\n", err_ret);
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
                    Serial.printf("IMU init err sh2_setSensorConfig 0x%02X\r\n", err_ret);
                    state_machine = ROACHIMU_SM_ERROR_WAIT;
                    error_time = millis();
                    break;
                }
                #ifdef ROACHIMU_ENABLE_DEBUG
                Serial.printf("IMU state machine started!\r\n");
                #endif
                reset_occured = false;
                state_machine = ROACHIMU_SM_SVC_START;
            }
            break;
        case ROACHIMU_SM_SVC_START:
            {
                if (
                #if defined(ESP32)
                    nbe_i2c_is_busy(&nbe_i2c) == false
                #elif defined(NRF52840_XXAA)
                    nbtwi_isBusy() == false
                #endif
                )
                {
                    state_machine = ROACHIMU_SM_SVC_GET_HEADER;
                    break;
                }
            }
            break;
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
                nbtwi_read(i2c_addr, len);
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
                        nbtwi_hasResult() || timeout
                    #endif
                    )
                {
                    if (
                        #if defined(ESP32)
                            nbe_i2c.error != 0
                        #elif defined(NRF52840_XXAA)
                            timeout
                        #endif
                        )
                    {
                        Serial.printf("IMU timeout header_wait [%u, %u, %u]\r\n", millis(), read_time, state_machine);
                        state_machine = ROACHIMU_SM_ERROR;
                        break;
                    }
                    #if defined(NRF52840_XXAA)
                    nbtwi_readResult(rx_buff_i2c, 4, true);
                    #endif

                    uint8_t* header = (uint8_t*)rx_buff_i2c;
                    
                    // Determine amount to read
                    uint16_t packet_size = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
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
                nbtwi_read(i2c_addr, len);
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
                        nbtwi_hasResult() || timeout
                    #endif
                    )
                {
                    if (
                        #if defined(ESP32)
                            nbe_i2c.error != 0
                        #elif defined(NRF52840_XXAA)
                            timeout
                        #endif
                        )
                    {
                        Serial.printf("IMU timeout chunk_wait [%u, %u, %u]\r\n", millis(), read_time, state_machine);
                        state_machine = ROACHIMU_SM_ERROR;
                        break;
                    }
                    #if defined(NRF52840_XXAA)
                    nbtwi_readResult(rx_buff_i2c, svc_reader.read_size, true);
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
    #ifdef ROACHIMU_ENABLE_DEBUG
    //Serial.printf("IMU service paused %u\r\n", millis());
    #endif
}

void RoachIMU::tare(void)
{
    pause_service();
    sh2_setTareNow(0x07, (sh2_TareBasis_t)0x05); // tare all three axis, for gyro-integrated-RV
}

void RoachIMU::doMath(void)
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
                memcpy((void*)&(euler), (void*)&(eu), sizeof(euler_t));
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
            default:
                memcpy((void*)&(euler), (void*)&(eu), sizeof(euler_t));
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

bool RoachIMU::i2c_write(uint8_t* buf, int len)
{
    #ifdef ROACHIMU_ENABLE_DEBUG
    //Serial.printf("RoachIMU::i2c_write %u\r\n", len);
    #endif

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
    nbtwi_write(i2c_addr, buf, len, false);
    nbtwi_transfer();
    return nbtwi_lastError() == 0;
    #endif
}

bool RoachIMU::i2c_read(uint8_t* buf, int len)
{
    #ifdef ROACHIMU_ENABLE_DEBUG
    //Serial.printf("RoachIMU::i2c_read %u\r\n", len);
    #endif

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
    nbtwi_read(i2c_addr, len);
    nbtwi_transfer();
    if (nbtwi_hasResult()) {
        nbtwi_readResult(rx_buff_i2c, len, true);
    }
    else {
        Serial.printf("RoachIMU::i2c_read err no result\r\n");
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

    #ifdef ROACHIMU_ENABLE_DEBUG
    Serial.printf("RoachIMU i2chal_read %u\r\n", len);
    #endif

    uint8_t header[4];
    if (!instance->i2c_read(header, 4)) {
        Serial.printf("i2chal_read err\r\n");
        return 0;
    }

    // Determine amount to read
    uint16_t packet_size = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
    // Unset the "continue" bit
    packet_size &= ~0x8000;

    size_t i2c_buffer_max = ROACHIMU_BUFF_RX_SIZE;

    if (packet_size > len) {
        // packet wouldn't fit in our buffer
        Serial.printf("ERR[%u]: i2chal_read packet size too big %d\r\n", millis(), packet_size);
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
    #ifdef ROACHIMU_ENABLE_DEBUG
    Serial.printf("RoachIMU i2chal_write %u\r\n", len);
    #endif

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
        Serial.printf("IMU hal_callback SH2_RESET %u\r\n", millis());
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
