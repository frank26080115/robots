#ifndef _ROACHIMU_BNO_H_
#define _ROACHIMU_BNO_H_

#include <Arduino.h>

#include <RoachLib.h>

#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"

#if defined(ESP32)
#include "nbe_i2c.h"
#elif defined(NRF52840_XXAA)
#include <NonBlockingTwi.h>
#include <Adafruit_TinyUSB.h>
#endif

#define BNO08x_I2CADDR_DEFAULT 0x4A

#define ROACHIMU_BUFF_TX_SIZE 512
#define ROACHIMU_BUFF_RX_SIZE 512

#define ROACHIMU_DEF_PIN_SDA 22
#define ROACHIMU_DEF_PIN_SCL 23

#define ROACHIMU_EXTRA_DATA

typedef struct
{
    int cargo_remaining;
    int cargo_read_amount;
    bool first_read;
    int read_size;
}
svc_state_t;

enum
{
    ROACHIMU_SM_SETUP,
    ROACHIMU_SM_SETUP_RST1,
    ROACHIMU_SM_SETUP_RST2,
    ROACHIMU_SM_SETUP_RST3,
    ROACHIMU_SM_SETUP_2,
    ROACHIMU_SM_SETUP_3,
    ROACHIMU_SM_SETUP_F,
    ROACHIMU_SM_SETUP_4,
    ROACHIMU_SM_SVC_START,
    ROACHIMU_SM_SVC_GET_HEADER,
    ROACHIMU_SM_SVC_GET_HEADER_WAIT,
    ROACHIMU_SM_SVC_READ_CHUNK,
    ROACHIMU_SM_SVC_READ_CHUNK_WAIT,
    ROACHIMU_SM_SVC_READ_DONE,
    ROACHIMU_SM_SVC_READ_DONE_LOOP,
    ROACHIMU_SM_SVC_READ_DONE_WAIT,
    ROACHIMU_SM_ERROR,
    ROACHIMU_SM_ERROR_WAIT,
};

class RoachIMU_BNO : public RoachIMU_Common
{
    public:
        RoachIMU_BNO(int samp_interval   = 5000,
                     int rd_interval     = 0,
                     int orientation     = 0,
                     int dev_addr        = BNO08x_I2CADDR_DEFAULT,
                     int rst             = -1
                    );
        virtual void begin(void);
        virtual void task(void);

        void pause_service(void);
        void tare(void);

        bool i2c_write(uint8_t* buf, int len);
        bool i2c_read (uint8_t* buf, int len);
        void sensorHandler(sh2_SensorEvent_t*);

        int pin_rst;
        bool reset_occured;
        sh2_GyroIntegratedRV_t girv;
        sh2_ProductIds_t prodIds;
        #ifdef ROACHIMU_EXTRA_DATA
        sh2_Accelerometer_t accelerometer;
        sh2_Accelerometer_t accelerometerRaw;
        sh2_Gyroscope_t gyroscope;
        sh2_GyroscopeUncalibrated_t gyroscopeUncal;
        sh2_RawGyroscope_t gyroscopeRaw;
        #endif

    protected:
        virtual void writeEuler(euler_t*);

    private:
        #if defined(ESP32)
        nbe_i2c_t nbe_i2c;
        #elif defined(NRF52840_XXAA)
        #endif

        sh2_Hal_t hal;
        svc_state_t svc_reader;
        uint8_t tx_buff[ROACHIMU_BUFF_TX_SIZE];
        uint8_t rx_buff_i2c[ROACHIMU_BUFF_RX_SIZE]; // used by the nbe library, raw single i2c transaction data
        uint8_t rx_buff_sh2[ROACHIMU_BUFF_RX_SIZE]; // used by the sh2 library, assembled packets
        int     rx_buff_wptr;                       // write pointer for rx_buff_sh2
        int read_interval;
        sh2_SensorValue_t sensor_value;
};

#endif
