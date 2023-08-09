#ifndef _ROACHIMU_LSM_H_
#define _ROACHIMU_LSM_H_

#include <Arduino.h>

#include <RoachLib.h>
#include "RoachMahony.h"

#if defined(ESP32)
#error ESP32 not supported for LSM6DS3
#elif defined(NRF52840_XXAA)
#include <NonBlockingTwi.h>
#include <Adafruit_TinyUSB.h>
#endif

#define LSM6DS3_I2CADDR_DEFAULT 0x6A

#define ROACHIMU_BUFF_TX_SIZE 512
#define ROACHIMU_BUFF_RX_SIZE 512

#define ROACHIMU_DEF_PIN_SDA 17
#define ROACHIMU_DEF_PIN_SCL 16
#define ROACHIMU_DEF_PIN_PWR 15
#define ROACHIMU_DEF_PIN_INT 18

#define ROACHIMU_EXTRA_DATA

enum
{
    ROACHIMU_SM_SETUP,
    ROACHIMU_SM_SETUP_WAIT,
    ROACHIMU_SM_RUN,
    ROACHIMU_SM_RUN_WAIT,
    ROACHIMU_SM_ERROR,
};

class RoachIMU_LSM : public RoachIMU_Common
{
    public:
        RoachIMU_LSM(int orientation     = 0,
                     int dev_addr        = LSM6DS3_I2CADDR_DEFAULT,
                     int pwr_pin         = ROACHIMU_DEF_PIN_PWR,
                     int irq_pin         = ROACHIMU_DEF_PIN_INT
                    );
        virtual void begin(void);
        virtual void task(void);

        void tare(void);

    protected:
        virtual void writeEuler(euler_t*);

    private:
        RoachMahony* ahrs;
        #if defined(ESP32)
        nbe_i2c_t nbe_i2c;
        #elif defined(NRF52840_XXAA)
        #endif
        uint8_t i2c_addr;
        uint8_t tx_buff[64];
        uint8_t rx_buff[64];
        uint8_t init_idx;
        uint32_t sample_time, read_time, error_time;
        int pin_irq, pin_pwr;
};

#endif
