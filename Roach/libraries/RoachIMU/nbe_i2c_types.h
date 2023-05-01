#pragma once

#include "nbe_i2c_defs.h"
#include <soc/i2c_struct.h>

typedef struct {
    i2c_dev_t *dev;
    uint32_t version;
} i2c_hal_context_t;

typedef union {
    struct {
        uint32_t byte_num:    8,
                 ack_en:      1,
                 ack_exp:     1,
                 ack_val:     1,
                 op_code:     3,
                 reserved14: 17,
                 done:        1;
    };
    uint32_t val;
} i2c_hw_cmd_t;

typedef int i2c_port_t;

typedef enum {
    I2C_SCLK_DEFAULT = 0,    /*!< I2C source clock not selected*/
//#if SOC_I2C_SUPPORT_APB
    I2C_SCLK_APB,            /*!< I2C source clock from APB, 80M*/
//#endif
#if SOC_I2C_SUPPORT_XTAL
    I2C_SCLK_XTAL,           /*!< I2C source clock from XTAL, 40M */
#endif
#if SOC_I2C_SUPPORT_RTC
    I2C_SCLK_RTC,            /*!< I2C source clock from 8M RTC, 8M */
#endif
#if SOC_I2C_SUPPORT_REF_TICK
    I2C_SCLK_REF_TICK,       /*!< I2C source clock from REF_TICK, 1M */
#endif
    I2C_SCLK_MAX,
} i2c_sclk_t;
