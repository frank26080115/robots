#include "RoachIMU.h"
#include "RoachIMU_LSM.h"

static const uint8_t lsm6ds3_init_table[][] = 
{
    { LSM6DS3_REG_CTRL1_XL, 0x4C }, // set the Accelerometer control register to work at 104 Hz, 4 g,and in bypass mode and enable ODR/4
    { LSM6DS3_REG_CTRL2_G , 0x4A }, // set the gyroscope control register to work at 104 Hz, 2000 dps and in bypass mode
    { LSM6DS3_REG_CTRL8_XL, 0x09 }, // set the ODR config register to ODR/4
    { LSM6DS3_REG_CTRL7_G , 0x00 }, // set gyroscope power mode to high performance and bandwidth to 16 MHz

    { LSM6DS3_REG_CTRL10_C, 0x44 }, // enable BDU block update, enable auto-inc address

    #ifdef LSM6DS3_USE_FIFO
    { LSM6DS3_REG_FIFO_CTRL1, 0x06 },   // FIFO threshold low
    { LSM6DS3_REG_FIFO_CTRL2, 0x00 },   // FIFO threshold high
    { LSM6DS3_REG_FIFO_CTRL3, 0x09 },   // enable both gyro and accel, no decimation
    //{ LSM6DS3_REG_FIFO_CTRL4, 0x00 }, // FIFO_CTRL4 not used
    { LSM6DS3_REG_FIFO_CTRL5, 0x26 },   // 104 Hz, continuous mode
    { LSM6DS3_REG_ORIENT_CFG_G, 0x00 }, // dataready in latched mode
    { LSM6DS3_REG_INT1_CTRL, 0x08 },    // interrupt on FIFO threshold
    #else
    { LSM6DS3_REG_INT1_CTRL, 0x03 },    // interrupt on gyro and accel
    #endif

    { 0xFF, 0xFF }, // end of table
};
