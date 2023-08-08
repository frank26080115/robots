#ifndef _ROACHIMU_LSM_H_
#define _ROACHIMU_LSM_H_

#include <Arduino.h>

#include <RoachLib.h>

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
#define ROACHIMU_AUTO_MATH

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
    ROACHIMU_SM_RUN,
    ROACHIMU_SM_ERROR,
    ROACHIMU_SM_ERROR_WAIT,
};

class RoachIMU_LSM
{
    public:
        RoachIMU_LSM(int samp_interval   = 5000,
                     int rd_interval     = 0,
                     int orientation     = 0,
                     int dev_addr        = LSM6DS3_I2CADDR_DEFAULT,
                    );
        void begin(void);
        void task(void);

        bool is_ready;
        bool has_new;
        bool is_inverted;
        float heading;
        uint8_t install_orientation;
        int total_cnt = 0;

        void pause_service(void);
        void doMath(void);
        void tare(void);

        // hasFailed will indicate if the IMU is reliable, this should be permanent and not recoverable
        inline bool hasFailed(void) { return fail_cnt > 3 || perm_fail > 3; };

        inline uint32_t totalFails(void) { return total_fails; };
        inline uint32_t getTotal(void) { return total_cnt; };

        // error occured signals that the heading track reference needs to be reset
        inline bool getErrorOccured(bool clr) { bool x = err_occured; if (clr) { err_occured = false; } return x || hasFailed(); };

        euler_t euler;

        uint8_t state_machine;
        int pin_sda, pin_scl, pin_rst;

    private:
        #if defined(ESP32)
        nbe_i2c_t nbe_i2c;
        #elif defined(NRF52840_XXAA)
        #endif
        uint8_t i2c_addr;
        uint8_t init_idx;
        uint32_t sample_time, read_time, error_time;
        int sample_interval, read_interval, calc_interval;
        int err_cnt, fail_cnt = 0, total_fails = 0, rej_cnt = 0;
        bool err_occured = false;
        int perm_fail = 0;

        euler_t* euler_filter = NULL;
};

#endif
