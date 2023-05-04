#ifndef _ROACH_CONF_H_
#define _ROACH_CONF_H_

#include <stdint.h>
#include <stdbool.h>

#define ROACH_SCALE_MULTIPLIER 1000
#define ROACH_FILTER_DEFAULT   (ROACH_SCALE_MULTIPLIER - (ROACH_SCALE_MULTIPLIER / 10))

#define ROACH_ADC_MAX   4095
#define ROACH_ADC_MID   2048
#define ROACH_ADC_NOISE 32

#endif
