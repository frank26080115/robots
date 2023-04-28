#ifndef _ROACHIMU_H_
#define _ROACHIMU_H_

#include <Arduino.h>
#include "sh2.h"
#include "nbe_i2c.h"

#define ROACHIMU_BUFF_TX_SIZE 512
#define ROACHIMU_BUFF_RX_SIZE 512

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
    ROACHIMU_SM_SVC_GET_HEADER,
    ROACHIMU_SM_SVC_GET_HEADER_WAIT,
    ROACHIMU_SM_SVC_READ_CHUNK,
    ROACHIMU_SM_SVC_READ_CHUNK_WAIT,
    ROACHIMU_SM_SVC_READ_DONE,
    ROACHIMU_SM_SVC_READ_DONE_LOOP,
    ROACHIMU_SM_SVC_READ_DONE_WAIt,
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
    ROACHIMU_ORIENTATION_FLIP_YAW   = 0x20,
};

typedef struct
{
    float yaw;
    float pitch;
    float roll;
} euler_t;

class RoachIMU
{
    public:
        RoachIMU(int samp_interval = 10000, int rd_interval = 0, int orientation = 0, int rst = -1, int sda = GPIO_NUM_22, int scl = GPIO_NUM_23);
        void begin(void);
        void task(void);

        bool is_ready;
        bool has_new;
        bool is_inverted;
        float heading;
        uint8_t install_orientation;

        void pause_service(void);
        void do_math(void);
        void tare(void);

        bool reset_occured;
        sh2_SensorValue_t sensor_value;
        sh2_GyroIntegratedRV_t girv;
        euler_t euler;
        sh2_ProductIds_t prodIds;

    private:
        int pin_sda, pin_scl, pin_rst;
        uint8_t state_machine;
        nbe_i2c_t nbe_i2c;
        sh2_Hal_t hal;
        svc_state_t svc_reader;
        uint8_t tx_buff[ROACHIMU_BUFF_TX_SIZE];
        uint8_t rx_buff_nbe[ROACHIMU_BUFF_RX_SIZE]; // used by the nbe library, raw single i2c transaction data
        uint8_t rx_buff_sh2[ROACHIMU_BUFF_RX_SIZE]; // used by the sh2 library, assembled packets
        int     rx_buff_wptr;                       // write pointer for rx_buff_sh2
        uint32_t sample_time, read_time, error_time;
        int sample_interval, read_interval;
};

#endif