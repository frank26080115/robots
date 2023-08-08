#ifndef _ROACHIMU_H_
#define _ROACHIMU_H_

#include <Arduino.h>

// configure which IMU to use here:

#define ROACHIMU_USE_BNO085
//#define ROACHIMU_USE_LSM6DS3

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

// conditionally include the right class and rename

#ifdef ROACHIMU_USE_BNO085
#include "RoachIMU_BNO.h"
#define RoachIMU RoachIMU_BNO
#endif

#ifdef ROACHIMU_USE_LSM6DS3
#include "RoachIMU_LSM.h"
#define RoachIMU RoachIMU_LSM
#endif

#endif
