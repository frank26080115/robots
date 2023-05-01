#pragma once

#include "nbe_i2c_defs.h"
#include "nbe_i2c_types.h"

#include <soc/soc.h>
#include <hal/misc.h>

/**
 * @brief  Get I2C txFIFO writable length
 *
 * @param  hw Beginning address of the peripheral registers
 *
 * @return TxFIFO writable length
 */
__attribute__((always_inline))
static inline uint32_t i2c_ll_get_txfifo_len(i2c_dev_t *hw)
{
    return SOC_I2C_FIFO_LEN - hw->status_reg.tx_fifo_cnt;
}

/**
 * @brief Get the rxFIFO readable length
 *
 * @param  hw Beginning address of the peripheral registers
 *
 * @return RxFIFO readable length
 */
__attribute__((always_inline))
static inline uint32_t i2c_ll_get_rxfifo_cnt(i2c_dev_t *hw)
{
    return hw->status_reg.rx_fifo_cnt;
}

/**
 * @brief  Write the I2C hardware txFIFO
 *
 * @param  hw Beginning address of the peripheral registers
 * @param  ptr Pointer to data buffer
 * @param  len Amount of data needs to be writen
 *
 * @return None.
 */
__attribute__((always_inline))
static inline void i2c_ll_write_txfifo(i2c_dev_t *hw, uint8_t *ptr, uint8_t len)
{
    uint32_t fifo_addr = (hw == &I2C0) ? 0x6001301c : 0x6002701c;
    for(int i = 0; i < len; i++) {
        WRITE_PERI_REG(fifo_addr, ptr[i]);
    }
}

/**
 * @brief  Read the I2C hardware rxFIFO
 *
 * @param  hw Beginning address of the peripheral registers
 * @param  ptr Pointer to data buffer
 * @param  len Amount of data needs read
 *
 * @return None
 */
__attribute__((always_inline))
static inline void i2c_ll_read_rxfifo(i2c_dev_t *hw, uint8_t *ptr, uint8_t len)
{
    for(int i = 0; i < len; i++) {
        ptr[i] = HAL_FORCE_READ_U32_REG_FIELD(hw->fifo_data, data);
    }
}

/**
 * @brief  Write I2C cmd register
 *
 * @param  hal Context of the HAL layer
 * @param  cmd I2C hardware command
 * @param  cmd_idx The index of the command register, should be less than 16
 *
 * @return None
 */
#define i2c_hal_write_cmd_reg(hal,cmd, cmd_idx)    i2c_ll_write_cmd_reg((hal)->dev,cmd,cmd_idx)

/**
 * @brief Write I2C hardware command register
 *
 * @param  hw Beginning address of the peripheral registers
 * @param  cmd I2C hardware command
 * @param  cmd_idx The index of the command register, should be less than 16
 *
 * @return None
 */
__attribute__((always_inline))
static inline void i2c_ll_write_cmd_reg(i2c_dev_t *hw, i2c_hw_cmd_t cmd, int cmd_idx)
{
    hw->command[cmd_idx].val = cmd.val;
}

/**
 * @brief  Write the I2C rxfifo with the given length
 *
 * @param  hal Context of the HAL layer
 * @param  wr_data Pointer to data buffer
 * @param  wr_size Amount of data needs write
 *
 * @return None
 */
#define i2c_hal_write_txfifo(hal,wr_data,wr_size)    i2c_ll_write_txfifo((hal)->dev,wr_data,wr_size)

/**
 * @brief  Configure the I2C to triger a transaction
 *
 * @param  hal Context of the HAL layer
 *
 * @return None
 */
#define i2c_hal_trans_start(hal)    i2c_ll_trans_start((hal)->dev)

/**
 * @brief  Start I2C transfer
 *
 * @param  hw Beginning address of the peripheral registers
 *
 * @return None
 */
__attribute__((always_inline))
static inline void i2c_ll_trans_start(i2c_dev_t *hw)
{
    hw->ctr.trans_start = 1;
}
