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

#define ROACHIMU_DEF_PIN_SDA     PIN_WIRE1_SDA
#define ROACHIMU_DEF_PIN_SCL     PIN_WIRE1_SCL
#define ROACHIMU_DEF_PIN_PWR     PIN_LSM6DS3TR_C_POWER
#define ROACHIMU_DEF_PIN_INT     PIN_LSM6DS3TR_C_INT1

#define ROACHIMU_EXTRA_DATA
//#define ROACHIMU_AUTO_MATH

enum
{
    ROACHIMU_SM_PWR,
    ROACHIMU_SM_SETUP,
    ROACHIMU_SM_SETUP_WAIT,
    ROACHIMU_SM_RUN,
    ROACHIMU_SM_RUN_WAIT1,
    ROACHIMU_SM_RUN_WAIT2,
    ROACHIMU_SM_ERROR,
    ROACHIMU_SM_PWROFF,
    ROACHIMU_SM_I2CRESET,
};

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t z;
    float roll;
    float pitch;
    float yaw;
}
__attribute__ ((packed))
imu_lsm_cal_t;

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
        imu_lsm_cal_t* calib = NULL;

    protected:
        virtual void writeEuler(euler_t*);

    private:
        RoachMahony* ahrs;
        uint8_t i2c_addr;
        uint8_t tx_buff[64];
        uint8_t rx_buff[64];
        uint8_t init_idx;
        uint32_t sample_time, read_time, error_time;
        int pin_irq, pin_pwr;
        uint32_t pwr_time;

        // these are for calibration
        uint32_t tare_time = 0, tare_cnt = 0;
        int32_t tare_x = 0, tare_y = 0, tare_z = 0;
};

void XiaoBleSenseLsm_powerOn(int pin);

#endif
