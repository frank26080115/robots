#pragma once

#include "nbe_i2c_defs.h"
#include "nbe_i2c_types.h"

void i2c_hal_set_bus_timing(i2c_hal_context_t *hal, int scl_freq, i2c_sclk_t src_clk);
