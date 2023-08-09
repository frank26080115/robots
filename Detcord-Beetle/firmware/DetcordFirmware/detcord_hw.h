#ifndef _DETCORD_HW_H_
#define _DETCORD_HW_H_

#include <stdint.h>
#include <stdbool.h>

#define BOARD_IS_ITSYBITSY
//#define BOARD_IS_XIAOBLE

#ifdef BOARD_IS_ITSYBITSY

#define DETCORDHW_PIN_LED 3
#define DETCORDHW_PIN_BTN 4

#define DETCORDHW_PIN_I2C_SDA 22
#define DETCORDHW_PIN_I2C_SCL 23

#define DETCORDHW_PIN_IMU_RST     7
#define DETCORDHW_PIN_SERVO_WEAP  9
#define DETCORDHW_PIN_SERVO_DRV_R 10
#define DETCORDHW_PIN_SERVO_DRV_L 11
#define DETCORDHW_PIN_ADC_BATT    A0

#endif

#ifdef BOARD_IS_XIAOBLE

#define DETCORDHW_PIN_LED_R 11
#define DETCORDHW_PIN_LED_G 13
#define DETCORDHW_PIN_LED_B 12

// note: RoachIMU_LSM has pin definitions for IMU already, it's on an internal bus

#define DETCORDHW_PIN_SERVO_WEAP  4
#define DETCORDHW_PIN_SERVO_DRV_R 5
#define DETCORDHW_PIN_SERVO_DRV_L 6
#define DETCORDHW_PIN_ADC_BATT    A3

#endif

#endif
