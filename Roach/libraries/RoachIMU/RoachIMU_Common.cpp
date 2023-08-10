#include "RoachIMU.h"

void RoachIMU_Common::doMath(void)
{
    #ifndef ROACHIMU_AUTO_MATH
    if (has_new)
    #endif
    {
        #ifndef ROACHIMU_AUTO_MATH
        has_new = false;
        #endif

        euler_t eu;
        writeEuler(&eu);
        // BNO085 will just copy from the event data
        // LSM6DS3 will run AHRS

        #ifdef ROACHIMU_REJECT_OUTLIERS
        bool reject = false;
        if (euler_filter == NULL) // filter has no data, so just set the first ever sample
        {
            euler_filter = (euler_t*)malloc(sizeof(euler_t));
            memcpy((void*)euler_filter, (void*)&(eu), sizeof(euler_t));
            rej_cnt = 0;
        }
        else
        {
            // track a low-pass filtered version of the euler angles
            // in order to detect outlier packets
            float *f = (float*)&(euler_filter->yaw), *eup = (float*)&(eu.yaw);
            int i, bad_axis = 0;
            for (i = 0; i < 3; i++)
            {
                if (f[i] < 0 && eup[i] >= 0) {
                    f[i] += 360;
                }
                else if (f[i] >= 0 && eup[i] < 0) {
                    f[i] -= 360;
                }
                float d = f[i] - eup[i];
                const float alpha = 0.2;
                f[i] = (f[i] * (1.0f - alpha)) + (eup[i] * alpha);
                ROACH_WRAP_ANGLE(d, 1);
                if (abs(d) > ((float)(360 * 20) / (float)(1000 / calc_interval))) {
                    bad_axis++;
                }
                ROACH_WRAP_ANGLE(f[i], 1);
            }
            reject = (bad_axis >= 2);
        }

        if (reject)
        {
            if (rej_cnt > (1000 / calc_interval))
            {
                // too many rejected samples, the IMU is going nuts
                // put it into a failure state so it can reboot
                rej_cnt = 0;
                error_time = millis();
                fail_cnt++;
                perm_fail++;
                state_machine = ROACHIMU_SM_ERROR_WAIT;
            }
            else {
                rej_cnt++;
            }
            return;
        }
        rej_cnt  = (rej_cnt > 0) ? (rej_cnt - 1) : 0;
        #endif
        err_cnt  = 0;
        fail_cnt = 0;

        // reorder the euler angles according to installation orientation
        uint8_t ori = install_orientation & 0x0F;
        memcpy((void*)&(euler), (void*)&(eu), sizeof(euler_t));
        switch (ori)
        {
            case ROACHIMU_ORIENTATION_XYZ:
                break;
            case ROACHIMU_ORIENTATION_XZY:
                euler.roll -= 90;
                break;
            case ROACHIMU_ORIENTATION_YXZ:
                euler.roll  = eu.pitch;
                euler.pitch = eu.roll;
                euler.yaw   = eu.yaw;
                break;
            case ROACHIMU_ORIENTATION_YZX:
                euler.roll  -= 90;
                euler.pitch -= 90;
                break;
            case ROACHIMU_ORIENTATION_ZXY:
                euler.roll  += 90;
                break;
            case ROACHIMU_ORIENTATION_ZYX:
                euler.pitch -= 90;
                break;
            default:
                memcpy((void*)&(euler), (void*)&(eu), sizeof(euler_t));
                break;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_FLIP_ROLL) != 0) {
            euler.roll += 180;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_FLIP_PITCH) != 0) {
            euler.pitch += 180;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_REV_ROLL) != 0) {
            euler.roll = 180 - euler.roll;
        }
        if ((install_orientation & ROACHIMU_ORIENTATION_REV_PITCH) != 0) {
            euler.pitch = 180 - euler.pitch;
        }
        ROACH_WRAP_ANGLE(euler.yaw  , 1);
        ROACH_WRAP_ANGLE(euler.roll , 1);
        ROACH_WRAP_ANGLE(euler.pitch, 1);
        float invert_hysterisis = 2;
        invert_hysterisis *= is_inverted ? -1 : 1;
        is_inverted = (euler.roll > (90 + invert_hysterisis) || euler.roll < (-90 - invert_hysterisis)) || (euler.pitch > (90 + invert_hysterisis) || euler.pitch < (-90 - invert_hysterisis));
        heading = euler.yaw;
        //if (is_inverted) {
        //    heading *= -1;
        //}
    }
}

void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees = false)
{
    float sqr = sq(qr);
    float sqi = sq(qi);
    float sqj = sq(qj);
    float sqk = sq(qk);

    ypr->yaw   = atan2( 2.0 * (qi * qj + qk * qr) , ( sqi - sqj - sqk + sqr));
    ypr->pitch = asin (-2.0 * (qi * qk - qj * qr) / ( sqi + sqj + sqk + sqr));
    ypr->roll  = atan2( 2.0 * (qj * qk + qi * qr) , (-sqi - sqj + sqk + sqr));

    if (degrees) {
        ypr->yaw   *= RAD_TO_DEG;
        ypr->pitch *= RAD_TO_DEG;
        ypr->roll  *= RAD_TO_DEG;
    }
}
