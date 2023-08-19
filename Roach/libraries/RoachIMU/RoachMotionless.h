#ifndef _ROACHMOTIONLESS_H_
#define _ROACHMOTIONLESS_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define RIML_ABS(x)         (((x) < 0) ? (-(x)) : (x))
#define RIML_DELTA(x, p)    (((x) >= (p)) ? ((x) - (p)) : ((p) - (x)))
#define RIML_MAX(x, y)      (((x) > (y)) ? (x) : (y))
#define RIML_MIN(x, y)      (((x) < (y)) ? (x) : (y))
extern int roach_div_rounded(const int n, const int d);

class RoachMotionless
{
    public:
        RoachMotionless(
                              uint32_t accel_lim_abs   // instantaneous acceleration limit
                            , uint32_t accel_lim_sum   // limit on the sum of deltas
                            , uint32_t accel_lim_delta // limit on each individual delta
                            , uint32_t accel_lim_gap   // limit on difference between min and max
                            , uint32_t accel_lim_mag   // instantaneous 3 axis acceleration magnitude limit
                            , uint32_t gyro_lim_abs    // instantaneous gyro limit
                            , uint32_t gyro_lim_sum    // limit on the sum of deltas
                            , uint32_t gyro_lim_delta  // limit on each individual delta
                            , uint32_t gyro_lim_gap    // limit on difference between min and max
                            , uint32_t time_ms         // time to hold still in ms
                            , uint32_t min_samples     // minimum samples required
                            , uint32_t timewin_min     // time window millis min
                            , uint32_t timewin_max     // time window millis max
                            ) {
            _accel_lim_abs   = accel_lim_abs  ;
            _accel_lim_sum   = accel_lim_sum  ;
            _accel_lim_delta = accel_lim_delta;
            _accel_lim_gap   = accel_lim_gap  ;
            _accel_lim_mag   = accel_lim_mag  ;
            _gyro_lim_abs    = gyro_lim_abs   ;
            _gyro_lim_sum    = gyro_lim_sum   ;
            _gyro_lim_delta  = gyro_lim_delta ;
            _gyro_lim_gap    = gyro_lim_gap   ;
            _time_ms         = time_ms        ;
            _min_samples     = min_samples    ;
            _timewin_min     = timewin_min    ;
            _timewin_max     = timewin_max    ;
        };

        void begin(uint32_t now) {
            _time_start   = now;
            _sample_cnt   = 0;
            _dsum_accel   = 0;
            _dsum_gyro    = 0;
            _avg_gyro_x   = 0;
            _avg_gyro_y   = 0;
            _avg_gyro_z   = 0;
            _done = false;
        };

        void getAverageGyro(int32_t* x, int32_t* y, int32_t* z) {
            *x = _avg_gyro_x;
            *y = _avg_gyro_y;
            *z = _avg_gyro_z;
        };

        inline bool     isDone             (void) { return _done;        };
        inline uint16_t getLastReason      (void) { return _reason_code; };
        inline uint32_t getAccumAccelDeltas(void) { return _dsum_accel;  };
        inline uint32_t getAccumGyroDeltas (void) { return _dsum_gyro;   };

        bool update(uint32_t now, int32_t gx, int32_t gy, int32_t gz, int32_t ax, int32_t ay, int32_t az)
        {
            bool x = update_inner(now, gx, gy, gz, ax, ay, az);
            _prev_accel_x = ax;
            _prev_accel_y = ay;
            _prev_accel_z = az;
            _prev_gyro_x  = gx;
            _prev_gyro_y  = gy;
            _prev_gyro_z  = gz;
            if (x == false) {
                begin(now);
            }
            return x;
        };

        bool update_inner(uint32_t now, int32_t gx, int32_t gy, int32_t gz, int32_t ax, int32_t ay, int32_t az)
        {
            if (_done) {
                return true;
            }

            if (now > _timewin_max && _timewin_max > 0) {
                return false;
            }

            _reason_code = 0;

            #define RIML_CHECK_EXCEEDED(x)    if (x) { _reason_code = __LINE__; return false; }

            if (_sample_cnt == 0)
            {
                _accel_min_x = ax;
                _accel_max_x = ax;
                _accel_min_y = ay;
                _accel_max_y = ay;
                _accel_min_z = az;
                _accel_max_z = az;
                _gyro_min_x  = gx;
                _gyro_max_x  = gx;
                _gyro_min_y  = gy;
                _gyro_max_y  = gy;
                _gyro_min_z  = gz;
                _gyro_max_z  = gz;
                _prev_accel_x = ax;
                _prev_accel_y = ay;
                _prev_accel_z = az;
                _prev_gyro_x  = gx;
                _prev_gyro_y  = gy;
                _prev_gyro_z  = gz;
            }

            uint32_t aax = RIML_ABS(ax);
            uint32_t aay = RIML_ABS(ay);
            uint32_t aaz = RIML_ABS(az);
            uint32_t agx = RIML_ABS(gx);
            uint32_t agy = RIML_ABS(gy);
            uint32_t agz = RIML_ABS(gz);

            uint8_t accel_exceeded_abs = 0;
            if (aax > _accel_lim_abs) {
                accel_exceeded_abs++;
            }
            if (aay > _accel_lim_abs) {
                accel_exceeded_abs++;
            }
            if (aaz > _accel_lim_abs) {
                accel_exceeded_abs++;
            }

            RIML_CHECK_EXCEEDED(accel_exceeded_abs > 1);

            RIML_CHECK_EXCEEDED(agx > _gyro_lim_abs);
            RIML_CHECK_EXCEEDED(agy > _gyro_lim_abs);
            RIML_CHECK_EXCEEDED(agz > _gyro_lim_abs);

            uint32_t delta;
            delta = RIML_DELTA(ax, _prev_accel_x);
            RIML_CHECK_EXCEEDED(delta > _accel_lim_delta);
            _dsum_accel += delta;
            delta = RIML_DELTA(ay, _prev_accel_y);
            RIML_CHECK_EXCEEDED(delta > _accel_lim_delta);
            _dsum_accel += delta;
            delta = RIML_DELTA(az, _prev_accel_z);
            RIML_CHECK_EXCEEDED(delta > _accel_lim_delta);
            _dsum_accel += delta;
            RIML_CHECK_EXCEEDED(_dsum_accel > _accel_lim_sum && _accel_lim_sum > 0);

            delta = RIML_DELTA(gx, _prev_gyro_x);
            RIML_CHECK_EXCEEDED(delta > _gyro_lim_delta);
            _dsum_gyro += delta;
            delta = RIML_DELTA(gy, _prev_gyro_y);
            RIML_CHECK_EXCEEDED(delta > _gyro_lim_delta);
            _dsum_gyro += delta;
            delta = RIML_DELTA(gz, _prev_gyro_z);
            RIML_CHECK_EXCEEDED(delta > _gyro_lim_delta);
            _dsum_gyro += delta;
            RIML_CHECK_EXCEEDED(_dsum_gyro > _gyro_lim_sum && _gyro_lim_sum > 0);

            _accel_max_x = RIML_MAX(ax, _accel_max_x);
            _accel_max_y = RIML_MAX(ay, _accel_max_y);
            _accel_max_z = RIML_MAX(az, _accel_max_z);
            _accel_min_x = RIML_MIN(ax, _accel_min_x);
            _accel_min_y = RIML_MIN(ay, _accel_min_y);
            _accel_min_z = RIML_MIN(az, _accel_min_z);
            _gyro_max_x  = RIML_MAX(gx,  _gyro_max_x);
            _gyro_max_y  = RIML_MAX(gy,  _gyro_max_y);
            _gyro_max_z  = RIML_MAX(gz,  _gyro_max_z);
            _gyro_min_x  = RIML_MIN(gx,  _gyro_min_x);
            _gyro_min_y  = RIML_MIN(gy,  _gyro_min_y);
            _gyro_min_z  = RIML_MIN(gz,  _gyro_min_z);

            delta = _accel_max_x - _accel_min_x;
            RIML_CHECK_EXCEEDED(delta > _accel_lim_gap);
            delta = _accel_max_y - _accel_min_y;
            RIML_CHECK_EXCEEDED(delta > _accel_lim_gap);
            delta = _accel_max_z - _accel_min_z;
            RIML_CHECK_EXCEEDED(delta > _accel_lim_gap);

            delta = _gyro_max_x - _gyro_min_x;
            RIML_CHECK_EXCEEDED(delta > _gyro_lim_gap);
            delta = _gyro_max_y - _gyro_min_y;
            RIML_CHECK_EXCEEDED(delta > _gyro_lim_gap);
            delta = _gyro_max_z - _gyro_min_z;
            RIML_CHECK_EXCEEDED(delta > _gyro_lim_gap);

            uint32_t mag = 0;
            mag += aax * aax;
            mag += aay * aay;
            mag += aaz * aaz;
            mag = lround(sqrtf((float)mag));

            RIML_CHECK_EXCEEDED(mag > _accel_lim_mag);

            _avg_gyro_x += gx;
            _avg_gyro_y += gy;
            _avg_gyro_z += gz;
            _sample_cnt += 1;

            if ((now - _time_start) >= _time_ms && _sample_cnt >= _min_samples && now > _timewin_min) {
                _avg_gyro_x = roach_div_rounded(_avg_gyro_x, _sample_cnt);
                _avg_gyro_y = roach_div_rounded(_avg_gyro_y, _sample_cnt);
                _avg_gyro_z = roach_div_rounded(_avg_gyro_z, _sample_cnt);
                _good_time = now;
                _done = true;
                _reason_code = 0;
            }
            return true;
        };

    private:
        uint32_t _accel_lim_abs   ;
        uint32_t _accel_lim_sum   ;
        uint32_t _accel_lim_delta ;
        uint32_t _accel_lim_gap   ;
        uint32_t _accel_lim_mag   ;
        uint32_t _gyro_lim_abs    ;
        uint32_t _gyro_lim_sum    ;
        uint32_t _gyro_lim_delta  ;
        uint32_t _gyro_lim_gap    ;
        uint32_t _time_ms         ;
        uint32_t _min_samples     ;
        uint32_t _timewin_min     ;
        uint32_t _timewin_max     ;

        uint32_t _time_start;
        uint32_t _sample_cnt;
        bool _done;
        uint32_t _good_time;
        uint16_t _reason_code;

        int32_t _prev_accel_x;
        int32_t _prev_accel_y;
        int32_t _prev_accel_z;
        int32_t _prev_gyro_x ;
        int32_t _prev_gyro_y ;
        int32_t _prev_gyro_z ;

        uint32_t _dsum_accel;
        uint32_t _dsum_gyro;

        int32_t _accel_min_x;
        int32_t _accel_max_x;
        int32_t _accel_min_y;
        int32_t _accel_max_y;
        int32_t _accel_min_z;
        int32_t _accel_max_z;
        int32_t _gyro_min_x ;
        int32_t _gyro_max_x ;
        int32_t _gyro_min_y ;
        int32_t _gyro_max_y ;
        int32_t _gyro_min_z ;
        int32_t _gyro_max_z ;

        int32_t _avg_gyro_x;
        int32_t _avg_gyro_y;
        int32_t _avg_gyro_z;
};

#endif
