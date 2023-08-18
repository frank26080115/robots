#include "RoachIMU.h"

#ifdef ROACHIMU_USE_LSM6DS3

#include "RoachIMU_LSM.h"
#include "lsm_reg.h"

#define debug_printf(...)                 // do nothing
//#define debug_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__); } while (0)
//#define dbgcalib_printf(...)            // do nothing
#define dbgcalib_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__); } while (0)
//#define dbgerr_printf(...)            // do nothing
#define dbgerr_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__); } while (0)

static const uint8_t lsm6ds3_init_table[][2] = 
{
    { LSM6DS3_REG_CTRL4_C , 0x08 }, // enable DA timer
    { LSM6DS3_REG_CTRL8_XL, 0x09 }, // set the accel filter to ODR/4
    { LSM6DS3_REG_CTRL1_XL, 0x44 }, // set the accelerometer control register to work at 104 Hz, 16 g
    { LSM6DS3_REG_CTRL7_G , 0x00 }, // set gyroscope power mode to high performance and bandwidth to 16 MHz
    { LSM6DS3_REG_CTRL2_G , 0x4C }, // set the gyroscope control register to work at 104 Hz, 2000 dps and in bypass mode

    { LSM6DS3_REG_CTRL3_C , 0x44 }, // enable BDU block update, enable auto-inc address, interrupt pin active-high push-pull

    #ifdef LSM6DS3_USE_FIFO
    { LSM6DS3_REG_FIFO_CTRL1, 0x05 },   // FIFO threshold low
    { LSM6DS3_REG_FIFO_CTRL2, 0x00 },   // FIFO threshold high
    { LSM6DS3_REG_FIFO_CTRL3, 0x09 },   // enable both gyro and accel, no decimation
    //{ LSM6DS3_REG_FIFO_CTRL4, 0x00 }, // FIFO_CTRL4 not used
    { LSM6DS3_REG_FIFO_CTRL5, 0x26 },   // 104 Hz, continuous mode
    { LSM6DS3_REG_ORIENT_CFG_G, 0x00 }, // dataready in latched mode
    { LSM6DS3_REG_INT1_CTRL, 0x08 },    // interrupt on FIFO threshold
    #else
    { LSM6DS3_REG_INT1_CTRL, 0x02 },    // interrupt on gyro data
    #endif

    { 0xFF, 0xFF }, // end of table
};

typedef struct
{
    int16_t  gyro_x;
    int16_t  gyro_y;
    int16_t  gyro_z;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
}
__attribute__ ((packed))
lsm_data_t;

#define ROACHIMU_TARE_CNT 64

void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees);
extern int roach_div_rounded(const int n, const int d);

static RoachIMU_LSM* instance = NULL;

RoachIMU_LSM::RoachIMU_LSM(int orientation, int dev_addr, int pwr_pin, int irq_pin)
{
    install_orientation = orientation;
    i2c_addr = dev_addr;
    pin_irq = irq_pin;
    pin_pwr = pwr_pin;
    instance = this;
    is_ready = false;
    ahrs = new RoachMahony();
    motionless = new RoachMotionless(
        200,         // instantaneous acceleration limit
        0,           // limit on the sum of deltas
        12,          // limit on each individual delta
        16,          // limit on difference between min and max
        2250,        // instantaneous 3 axis acceleration magnitude limit
        32,          // instantaneous gyro limit
        0,           // limit on the sum of deltas
        4,           // limit on each individual delta
        8,           // limit on difference between min and max
        50 * 100,    // time to hold still
        500,         // minimum samples required
        3000,        // time window millis minimum
        20000        // time window millis maximum
    );
}

void RoachIMU_LSM::begin(void)
{
    if (pin_irq >= 0) {
        pinMode(pin_irq, INPUT);
        sample_interval = 9;
    }
    else
    {
        sample_interval = 11;
    }
    if (pin_pwr >= 0) {
        XiaoBleSenseLsm_powerOn(pin_pwr);
    }

    state_machine = ROACHIMU_SM_PWR;
    pwr_time = millis();
    init_idx = 0;
}

void RoachIMU_LSM::task(void)
{
    #if defined(NRF52840_XXAA)
    nbtwi_task();
    #endif

    switch (state_machine)
    {
        case ROACHIMU_SM_PWR:
            if ((millis() - pwr_time) >= 25) {
                state_machine = ROACHIMU_SM_SETUP;
                init_idx = 0;
                debug_printf("RoachIMU entering SETUP state\r\n");
            }
            break;
        case ROACHIMU_SM_SETUP:
            {
                ahrs->reset();
                fail_cnt = 0;
                has_new = false;
                tx_buff[0] = lsm6ds3_init_table[init_idx][0];
                tx_buff[1] = lsm6ds3_init_table[init_idx][1];
                if (tx_buff[0] == 0xFF)
                {
                    is_ready = true;
                    state_machine = ROACHIMU_SM_RUN;
                    sample_time = millis();
                    debug_printf("RoachIMU SETUP done -> RUN\r\n");
                    motionless->begin(sample_time);
                    break;
                }
                nbtwi_write(i2c_addr, (uint8_t*)tx_buff, 2, false);
                state_machine = ROACHIMU_SM_SETUP_WAIT;
                debug_printf("RoachIMU SETUP sending %u\r\n", init_idx);
            }
            break;
        case ROACHIMU_SM_SETUP_WAIT:
            {
                if (nbtwi_isBusy() == false)
                {
                    if (nbtwi_hasError(false) == false) {
                        debug_printf("RoachIMU SETUP sent %u\r\n", init_idx);
                        init_idx++;
                    }
                    else {
                        nbtwi_hasError(true);
                        dbgerr_printf("RoachIMU SETUP I2C error\r\n");
                        init_idx = 0;
                        perm_fail++;
                        err_occured = true;
                    }
                    if (perm_fail < 3) {
                        state_machine = ROACHIMU_SM_SETUP;
                    }
                    else {
                        state_machine = ROACHIMU_SM_ERROR;
                    }
                }
            }
            break;
        case ROACHIMU_SM_RUN:
            {
                uint32_t now = millis();
                bool to_read = false;
                bool can_read = (now - sample_time) >= (sample_interval - 1);
                if (pin_irq >= 0)
                {
                    if (digitalRead(pin_irq) != LOW && can_read)
                    {
                        to_read = true;
                        debug_printf("RoachIMU READ from INT\r\n");
                    }
                    else
                    {
                        to_read = (now - sample_time) >= sample_interval;
                        if (to_read)
                        {
                            debug_printf("RoachIMU READ from INT (timeout)\r\n");
                        }
                    }
                }
                else
                {
                    to_read = (now - sample_time) >= sample_interval;
                    if (to_read) {
                        debug_printf("RoachIMU READ from TIME\r\n");
                    }
                }

                if (to_read)
                {
                    sample_time = now;
                    #ifdef LSM6DS3_USE_FIFO
                    tx_buff[0] = LSM6DS3_REG_FIFO_DATA_OUT_L;
                    #else
                    tx_buff[0] = LSM6DS3_REG_OUTX_L_G;
                    #endif
                    nbtwi_write(i2c_addr, (uint8_t*)tx_buff, 1, false);
                    state_machine = ROACHIMU_SM_RUN_WAIT1;
                }
            }
            break;
        case ROACHIMU_SM_RUN_WAIT1:
        case ROACHIMU_SM_RUN_WAIT2:
            {
                bool suc = false;
                if (state_machine == ROACHIMU_SM_RUN_WAIT1)
                {
                    if (nbtwi_isBusy() == false)
                    {
                        if (nbtwi_hasError(false) == false)
                        {
                            debug_printf("RoachIMU READ addr sent\r\n");
                            nbtwi_read(i2c_addr, sizeof(lsm_data_t));
                            state_machine = ROACHIMU_SM_RUN_WAIT2;
                            suc = true;
                        }
                    }
                }
                else if (state_machine == ROACHIMU_SM_RUN_WAIT2)
                {
                    if (nbtwi_hasResult())
                    {
                        total_cnt++;
                        fail_cnt = 0;
                        state_machine = ROACHIMU_SM_RUN;

                        nbtwi_readResult((uint8_t*)rx_buff, sizeof(lsm_data_t), true);
                        debug_printf("RoachIMU RX'ed data\r\n");
                        has_new = true;
                        #ifdef ROACHIMU_AUTO_MATH
                        doMath();
                        has_new = true;
                        #endif
                        suc = true;
                    }
                }
                if (suc == false)
                {
                    if ((millis() - sample_time) >= 200)
                    {
                        nbtwi_hasError(true);
                        dbgerr_printf("RoachIMU READ I2C ERROR 0x%04X\r\n", nbtwi_lastError());
                        total_fails++;
                        if (fail_cnt < 2)
                        {
                            fail_cnt++;
                            state_machine = ROACHIMU_SM_RUN;
                        }
                        else
                        {
                            state_machine = ROACHIMU_SM_ERROR;
                        }
                    }
                }
            }
            break;
        case ROACHIMU_SM_ERROR:
            {
                err_occured = true;
                init_idx = 0;
                tare_time = 0;
                dbgerr_printf("RoachIMU too many errors\r\n");
                state_machine = ROACHIMU_SM_I2CRESET;
                pwr_time = millis();
                nbtwi_forceStop();
            }
            break;
        case ROACHIMU_SM_I2CRESET:
            {
                if (nbtwi_isBusy() == false)
                {
                    pwr_time = millis();
                    if (pin_pwr >= 0) {
                        state_machine = ROACHIMU_SM_PWROFF;
                        digitalWrite(pin_pwr, LOW);
                        dbgerr_printf("RoachIMU pwr off\r\n");
                    }
                    else {
                        state_machine = ROACHIMU_SM_PWR;
                    }
                }
            }
            break;
        case ROACHIMU_SM_PWROFF:
            {
                if ((millis() - pwr_time) >= 25) {
                    pwr_time = millis();
                    XiaoBleSenseLsm_powerOn(pin_pwr);
                    state_machine = ROACHIMU_SM_PWR;
                    dbgerr_printf("RoachIMU pwr on again\r\n");
                }
            }
            break;
    }
}

void RoachIMU_LSM::writeEuler(euler_t* eu)
{
    lsm_data_t* pkt = (lsm_data_t*)rx_buff;

    float accel_x = pkt->accel_x;
    float accel_y = pkt->accel_y;
    float accel_z = pkt->accel_z;

    // gyro is configured for 2000dps full-scale
    // gyro output specified 70 mdps/LSB

    int32_t gyro_x32 = pkt->gyro_x;
    int32_t gyro_y32 = pkt->gyro_y;
    int32_t gyro_z32 = pkt->gyro_z;

    bool need_cal = false;

    if (tare_time != 0) // calibration active
    {
        tare_x += gyro_x32;
        tare_y += gyro_y32;
        tare_z += gyro_z32;
        tare_cnt++;
        if (tare_cnt > 0 && (tare_cnt >= ROACHIMU_TARE_CNT
          || (millis() - tare_time) >= 500
        ))
        {
            tare_time = 0; // stop calibration
            if (calib == NULL) { // if missing, create new
                calib = (imu_lsm_cal_t*)malloc(sizeof(imu_lsm_cal_t));
            }
            if (calib != NULL) {
                dbgcalib_printf("IMU calib done [ %d , %d , %d ]", gyro_x32, gyro_y32, gyro_z32);
                dbgcalib_printf(" -> [ %d , %d , %d ] / %d", tare_x, tare_y, tare_z, tare_cnt);
                calib->x = roach_div_rounded(tare_x, tare_cnt);
                calib->y = roach_div_rounded(tare_y, tare_cnt);
                calib->z = roach_div_rounded(tare_z, tare_cnt);
                dbgcalib_printf(" = [ %d , %d , %d ] \r\n", calib->x, calib->y, calib->z);
                ahrs->reset();
                need_cal = true;
            }
        }
    }
    else if (tare_time == 0 && calib == NULL && motionless->isDone() == false)
    {
        bool mlsuc = motionless->update(millis(), pkt->gyro_x, pkt->gyro_y, pkt->gyro_z, pkt->accel_x, pkt->accel_y, pkt->accel_z);
        if (motionless->isDone())
        {
            if (calib == NULL) { // if missing, create new
                calib = (imu_lsm_cal_t*)malloc(sizeof(imu_lsm_cal_t));
            }
            if (calib != NULL) {
                int32_t cx, cy, cz;
                motionless->getAverageGyro(&cx, &cy, &cz);
                calib->x = cx;
                calib->y = cy;
                calib->z = cz;
                dbgcalib_printf("IMU motionless calib done [ %d , %d , %d ] \r\n", calib->x, calib->y, calib->z);
                ahrs->reset();
                need_cal = true;
            }
        }
        else if (mlsuc == false)
        {
            dbgcalib_printf("[%u] IMU not motionless %u %d %d %d %d %d %d\r\n", millis(), motionless->getLastReason()
            , pkt->accel_x
            , pkt->accel_y
            , pkt->accel_z
            , pkt->gyro_x
            , pkt->gyro_y
            , pkt->gyro_z
            );
        }
    }

    // apply calibration if available
    if (calib != NULL)
    {
        gyro_x32 -= calib->x;
        gyro_y32 -= calib->y;
        gyro_z32 -= calib->z;
    }

    // convert to units used by AHRS
    gyro_x32 *= 7;
    gyro_y32 *= 7;
    gyro_z32 *= 7;
    float gyro_x = gyro_x32;
    float gyro_y = gyro_y32;
    float gyro_z = gyro_z32;
    gyro_x /= 100.0;
    gyro_y /= 100.0;
    gyro_z /= 100.0;

    // accel data is unitless when used inside AHRS

    if (need_cal == false) {
        ahrs->updateIMU(gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z);
    }
    else {
        ahrs->updateIMU(0, 0, 0, accel_x, accel_y, accel_z);
    }

    // convert radians to degrees
    eu->roll  = ahrs->roll  * RAD_TO_DEG;
    eu->pitch = ahrs->pitch * RAD_TO_DEG;
    eu->yaw   = ahrs->yaw   * RAD_TO_DEG;

    if (need_cal) { // remember angle calibration
        calib->roll  = eu->roll;
        calib->pitch = eu->pitch;
        calib->yaw   = eu->yaw;
    }
    else if (calib != NULL) { // apply angle calibration if available
        eu->roll  -= calib->roll;
        eu->pitch -= calib->pitch;
        eu->yaw   -= calib->yaw;
    }
}

void RoachIMU_LSM::tare(void)
{
    // start calibration
    tare_time = millis();
    tare_cnt = 0;
    tare_x = 0;
    tare_y = 0;
    tare_z = 0;
}

static volatile uint32_t* pincfg_reg(uint32_t pin)
{
    NRF_GPIO_Type * port = nrf_gpio_pin_port_decode(&pin);
    return &port->PIN_CNF[pin];
}

void XiaoBleSenseLsm_powerOn(int pin)
{
    pinMode(pin, OUTPUT);
    *pincfg_reg(g_ADigitalPinMap[pin]) =
                                ((uint32_t)NRF_GPIO_PIN_DIR_OUTPUT << GPIO_PIN_CNF_DIR_Pos)
                              | ((uint32_t)NRF_GPIO_PIN_INPUT_DISCONNECT << GPIO_PIN_CNF_INPUT_Pos)
                              | ((uint32_t)NRF_GPIO_PIN_NOPULL << GPIO_PIN_CNF_PULL_Pos)
                              | ((uint32_t)NRF_GPIO_PIN_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
                              | ((uint32_t)NRF_GPIO_PIN_NOSENSE << GPIO_PIN_CNF_SENSE_Pos);
    digitalWrite(pin, HIGH);
}

#endif
