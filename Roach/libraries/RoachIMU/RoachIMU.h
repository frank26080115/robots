#ifndef _ROACHIMU_H_
#define _ROACHIMU_H_

#include <Arduino.h>

// configure which IMU to use here:

#define ROACHIMU_USE_BNO085
//#define ROACHIMU_USE_LSM6DS3

//#define ROACHIMU_AUTO_MATH

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

class RoachIMU_Common
{
    public:
        virtual void begin(void);
        virtual void task(void);

        // hasFailed will indicate if the IMU is reliable, this should be permanent and not recoverable
        inline bool hasFailed(void) { return fail_cnt > 3 || perm_fail > 3; };
        inline bool isReady  (void) { return is_ready; };
        inline bool hasNew(bool clr) { bool x = has_new; if (clr) { has_new = false; } return x; };

        inline uint32_t totalFails(void) { return total_fails; };
        inline uint32_t getTotal(void) { return total_cnt; };

        // error occured signals that the heading track reference needs to be reset
        inline bool getErrorOccured(bool clr) { bool x = err_occured; if (clr) { err_occured = false; } return x || hasFailed(); };

        void doMath(void);

        euler_t euler; // units in real degrees
        bool has_new;
        bool is_ready;
        bool is_inverted;
        float heading; // units in real degrees, one of the euler angles determined by the orientation
        uint8_t install_orientation;
        int total_cnt = 0;

    protected:
        virtual void writeEuler(euler_t*);
        uint8_t i2c_addr;
        uint8_t state_machine;
        int pin_sda, pin_scl;
        uint32_t sample_time, read_time, error_time;
        int sample_interval, calc_interval;
        int err_cnt, fail_cnt = 0, total_fails = 0, rej_cnt = 0;
        bool err_occured = false;
        int perm_fail = 0;
        euler_t* euler_filter = NULL;
};

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
