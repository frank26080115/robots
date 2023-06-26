#ifndef _ROACHIMU_H_
#define _ROACHIMU_H_

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

enum
{
    ROACHIMU_ORIENTATION_XYZ = 0,
    ROACHIMU_ORIENTATION_XZY,
    ROACHIMU_ORIENTATION_YZX,
    ROACHIMU_ORIENTATION_YXZ,
    ROACHIMU_ORIENTATION_ZXY,
    ROACHIMU_ORIENTATION_ZYX,
    ROACHIMU_ORIENTATION_FLIP_ROLL  = 0x80,
    ROACHIMU_ORIENTATION_FLIP_PITCH = 0x40,
    //ROACHIMU_ORIENTATION_FLIP_YAW   = 0x20,
};

typedef struct
{
    float yaw;
    float pitch;
    float roll;
}
__attribute__ ((packed))
euler_t;

class RoachIMU
{
    public:
        RoachIMU(int samp_interval   = 5000,
                 int rd_interval     = 0,
                 int orientation     = 0,
                 int dev_addr        = BNO08x_I2CADDR_DEFAULT,
                 int rst             = -1
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
        inline bool getErrorOccured(bool clr) { bool x = err_occured; if (clr) { err_occured = false; } return x; };

        bool i2c_write(uint8_t* buf, int len);
        bool i2c_read (uint8_t* buf, int len);

        bool reset_occured;
        sh2_SensorValue_t sensor_value;
        sh2_GyroIntegratedRV_t girv;
        euler_t euler;
        sh2_ProductIds_t prodIds;

        uint8_t state_machine;
        int pin_sda, pin_scl, pin_rst;

    private:
        #if defined(ESP32)
        nbe_i2c_t nbe_i2c;
        #elif defined(NRF52840_XXAA)
        #endif
        uint8_t i2c_addr;
        sh2_Hal_t hal;
        svc_state_t svc_reader;
        uint8_t tx_buff[ROACHIMU_BUFF_TX_SIZE];
        uint8_t rx_buff_i2c[ROACHIMU_BUFF_RX_SIZE]; // used by the nbe library, raw single i2c transaction data
        uint8_t rx_buff_sh2[ROACHIMU_BUFF_RX_SIZE]; // used by the sh2 library, assembled packets
        int     rx_buff_wptr;                       // write pointer for rx_buff_sh2
        uint32_t sample_time, read_time, error_time;
        int sample_interval, read_interval, calc_interval;
        int err_cnt, fail_cnt = 0, total_fails = 0, rej_cnt = 0;
        bool err_occured = false;
        int perm_fail = 0;

        euler_t* euler_filter = NULL;
};

#endif
