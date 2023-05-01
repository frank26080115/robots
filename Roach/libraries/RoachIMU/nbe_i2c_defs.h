#pragma once

// I2C operation mode command
#define I2C_LL_CMD_RESTART    0    /*!<I2C restart command, might be 6 */
#define I2C_LL_CMD_WRITE      1    /*!<I2C write command */
#define I2C_LL_CMD_READ       2    /*!<I2C read command */
#define I2C_LL_CMD_STOP       3    /*!<I2C stop command */
#define I2C_LL_CMD_END        4    /*!<I2C end command */

#define I2C_LL_GET_HW(i2c_num)        (((i2c_num) == 0) ? &I2C0 : &I2C1)

#define SOC_I2C_FIFO_LEN        (32) /*!< I2C hardware FIFO depth */
