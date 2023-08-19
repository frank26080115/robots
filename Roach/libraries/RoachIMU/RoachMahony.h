#ifndef _ROACHMAHONY_H_
#define _ROACHMAHONY_H_

#include <math.h>

#define ROACHMAHONY_DEFAULT_SAMPLE_FREQ 100.0f // sample frequency in Hz
#define twoKpDef (2.0f * 0.5f)     // 2 * proportional gain
#define twoKiDef (2.0f * 0.0f)     // 2 * integral gain

class RoachMahony
{
    public:
        RoachMahony(float samp_freq = ROACHMAHONY_DEFAULT_SAMPLE_FREQ, float prop_gain = twoKpDef, float int_gain = twoKiDef)
        {
            twoKp = prop_gain; // 2 * proportional gain (Kp)
            twoKi = int_gain;  // 2 * integral gain (Ki)
            q0 = 1.0f;
            q1 = 0.0f;
            q2 = 0.0f;
            q3 = 0.0f;
            integralFBx = 0.0f;
            integralFBy = 0.0f;
            integralFBz = 0.0f;
            invSampleFreq = 1.0f / samp_freq;
        };

        void reset(void)
        {
            q0 = 1.0f;
            q1 = 0.0f;
            q2 = 0.0f;
            q3 = 0.0f;
            integralFBx = 0.0f;
            integralFBy = 0.0f;
            integralFBz = 0.0f;
        };

        // gx gy gz expects units deg/sec
        // ax ay az expects unit-less, will be normalized to 1G internally
        // dt specified in seconds
        void updateIMU(float gx, float gy, float gz, float ax, float ay, float az, float dt = 0)
        {
            if (dt <= 0) {
                dt = invSampleFreq;
            }

            float recipNorm;
            float halfvx, halfvy, halfvz;
            float halfex, halfey, halfez;
            float qa, qb, qc;

            // Convert gyroscope degrees/sec to radians/sec
            gx *= DEG_TO_RAD;
            gy *= DEG_TO_RAD;
            gz *= DEG_TO_RAD;

            // Compute feedback only if accelerometer measurement valid
            // (avoids NaN in accelerometer normalisation)
            if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

                  // Normalise accelerometer measurement
                  recipNorm = invSqrt(ax * ax + ay * ay + az * az);
                  ax *= recipNorm;
                  ay *= recipNorm;
                  az *= recipNorm;

                  // Estimated direction of gravity
                  halfvx = q1 * q3 - q0 * q2;
                  halfvy = q0 * q1 + q2 * q3;
                  halfvz = q0 * q0 - 0.5f + q3 * q3;

                  // Error is sum of cross product between estimated
                  // and measured direction of gravity
                  halfex = (ay * halfvz - az * halfvy);
                  halfey = (az * halfvx - ax * halfvz);
                  halfez = (ax * halfvy - ay * halfvx);

                  // Compute and apply integral feedback if enabled
                  if (twoKi > 0.0f) {
                          // integral error scaled by Ki
                          integralFBx += twoKi * halfex * dt;
                          integralFBy += twoKi * halfey * dt;
                          integralFBz += twoKi * halfez * dt;
                          gx += integralFBx; // apply integral feedback
                          gy += integralFBy;
                          gz += integralFBz;
                  } else {
                          integralFBx = 0.0f; // prevent integral windup
                          integralFBy = 0.0f;
                          integralFBz = 0.0f;
                  }

                  // Apply proportional feedback
                  gx += twoKp * halfex;
                  gy += twoKp * halfey;
                  gz += twoKp * halfez;
            }

            // Integrate rate of change of quaternion
            gx *= (0.5f * dt); // pre-multiply common factors
            gy *= (0.5f * dt);
            gz *= (0.5f * dt);
            qa = q0;
            qb = q1;
            qc = q2;
            q0 += (-qb * gx - qc * gy - q3 * gz);
            q1 += (qa * gx + qc * gz - q3 * gy);
            q2 += (qa * gy - qb * gz + q3 * gx);
            q3 += (qa * gz + qb * gy - qc * gx);

            // Normalise quaternion
            recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
            q0 *= recipNorm;
            q1 *= recipNorm;
            q2 *= recipNorm;
            q3 *= recipNorm;

            roll  = atan2f(q0 * q1 + q2 * q3, 0.5f - q1 * q1 - q2 * q2);
            pitch = asinf(-2.0f * (q1 * q3 - q0 * q2));
            yaw   = atan2f(q1 * q2 + q0 * q3, 0.5f - q2 * q2 - q3 * q3);
            // these calculations output in radians

            grav[0] = 2.0f * (q1 * q3 - q0 * q2);
            grav[1] = 2.0f * (q0 * q1 + q2 * q3);
            grav[2] = 2.0f * (q1 * q0 - 0.5f + q3 * q3);
        };

        float roll, pitch, yaw; // radians

    private:
        float twoKp; // 2 * proportional gain (Kp)
        float twoKi; // 2 * integral gain (Ki)
        float q0, q1, q2, q3; // quaternion of sensor frame relative to auxiliary frame
        float integralFBx, integralFBy, integralFBz; // integral error terms scaled by Ki
        float invSampleFreq;
        float grav[3];

        float invSqrt(float x)
        {
            float halfx = 0.5f * x;
            float y = x;
            long i = *(long *)&y;
            i = 0x5f3759df - (i >> 1);
            y = *(float *)&i;
            y = y * (1.5f - (halfx * y * y));
            y = y * (1.5f - (halfx * y * y));
            return y;
        };
};

#endif