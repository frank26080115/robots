#include <RoachIMU.h>
#include <NonBlockingTwi.h>
#include <RoachUsbMsd.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>

#define USE_ASCII
//#define WAIT_BUTTON
#define WAIT_SERIAL

#ifdef USE_ASCII
#define FILE_EXT "txt"
#else
#define FILE_EXT "bin"
#endif

#define ROACHHW_PIN_BTN_D7   7

#define ROACHHW_PIN_LED_RED  3
#define ROACHHW_PIN_LED_BLU  4
#define ROACHHW_PIN_LED_NEO  8

#define ROACHHW_PIN_I2C_SCL 23
#define ROACHHW_PIN_I2C_SDA 22

RoachIMU imu(5000, 0, 0, BNO08x_I2CADDR_DEFAULT, -1);
char filename[64];
FatFile fatfile;

typedef struct
{
    uint32_t timestamp;
    int16_t  yaw;
    int16_t  roll;
    int16_t  pitch;
}
__attribute__ ((packed))
data_t;

void setup()
{
    pinMode(ROACHHW_PIN_BTN_D7, INPUT_PULLUP);

    pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
    pinMode(ROACHHW_PIN_LED_BLU, OUTPUT);
    digitalWrite(ROACHHW_PIN_LED_RED, LOW);
    digitalWrite(ROACHHW_PIN_LED_BLU, LOW);

    RoachUsbMsd_begin();
    if (RoachUsbMsd_hasVbus()) {
        RoachUsbMsd_presentUsbMsd();
    }

    #ifdef WAIT_SERIAL
    while (Serial.available() <= 0)
    {
        RoachUsbMsd_task();
        yield();
    }
    Serial.println("hello");
    #endif

    nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE);
    imu.begin();
    while (true)
    {
        RoachUsbMsd_task();
        nbtwi_task();
        imu.task();
        if (imu.has_new)
        {
            break;
        }
        yield();
    }
    Serial.println("IMU initialized");
    digitalWrite(ROACHHW_PIN_LED_RED, HIGH);

    if (RoachUsbMsd_isReady() == false)
    {
        error("filesys not ready");
    }

    int i, j;
    bool can_save = false;
    for (i = 1, j = 0; i <= 9999; i++)
    {
        sprintf(filename, "/%u." FILE_EXT, i);
        if (fatfile.open(&(filename[1]), O_RDONLY) == false)
        {
            j = i;
            fatfile.close();
            break;
        }
        fatfile.close();
    }

    if (j == 0)
    {
        error("cannot find file name to save");
    }

    Serial.printf("saving to %s\r\n", filename);

    #ifdef WAIT_BUTTON
    do
    {
        RoachUsbMsd_task();
        nbtwi_task();
        imu.task();
        if (digitalRead(ROACHHW_PIN_BTN_D7) == LOW) {
            break;
        }
        yield();
    }
    while (true);

    do
    {
        RoachUsbMsd_task();
        nbtwi_task();
        imu.task();
        if (digitalRead(ROACHHW_PIN_BTN_D7) != LOW) {
            break;
        }
        yield();
    }
    while (true);
    #endif

    RoachUsbMsd_unpresent();

    bool suc = fatfile.open(&filename[1], O_RDWR | O_CREAT);
    if (suc == false)
    {
        error("cannot open file");
    }

    digitalWrite(ROACHHW_PIN_LED_RED, LOW);
    digitalWrite(ROACHHW_PIN_LED_BLU, HIGH);
}

void loop()
{
    RoachUsbMsd_task();
    log_task();
}

void log_task()
{
    static int cnt = 0;
    nbtwi_task();
    imu.task();
    if (imu.has_new)
    {
        #ifndef ROACHIMU_AUTO_MATH
        imu.doMath();
        #endif
        imu.has_new = false;
        cnt++;
        if (digitalRead(ROACHHW_PIN_BTN_D7) == LOW) {
            fatfile.flush();
            return;
        }

        data_t d;
        d.timestamp = millis();
        if (imu.hasFailed()) {
            d.timestamp |= 0x40000000;
        }
        d.yaw   = lround(imu.euler.yaw   * 100);
        d.roll  = lround(imu.euler.roll  * 100);
        d.pitch = lround(imu.euler.pitch * 100);
        #ifndef USE_ASCII
        uint8_t* dp = (uint8_t*)&d;
        fatfile.write(dp, sizeof(data_t));
        #ifdef ROACHIMU_EXTRA_DATA
        fatfile.write((uint8_t*)&(imu.accelerometer), sizeof(sh2_Accelerometer_t));
        fatfile.write((uint8_t*)&(imu.gyroscope)    , sizeof(sh2_Gyroscope_t));
        #endif
        #else
        char str[2048];
        int len = sprintf(str, "%u, %d, %d, %d"
        #ifdef ROACHIMU_EXTRA_DATA
        ", %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f"
        #endif
        "\r\n"
        , d.timestamp, d.yaw, d.roll, d.pitch
        #ifdef ROACHIMU_EXTRA_DATA
        , imu.accelerometer.x, imu.accelerometer.y, imu.accelerometer.z, imu.gyroscope.x, imu.gyroscope.y, imu.gyroscope.z
        #endif
        );
        fatfile.write(str, len);
        #endif
        if ((cnt % 200) == 0) {
            Serial.printf("cnt %u\r\n", cnt);
            fatfile.flush();
        }
    }
    digitalWrite(ROACHHW_PIN_LED_BLU, (cnt % 100) < 20);
}

void error(const char* s)
{
    while (true)
    {
        RoachUsbMsd_task();
        Serial.println(s);
        digitalWrite(ROACHHW_PIN_LED_RED, HIGH);
        delay(300);
        digitalWrite(ROACHHW_PIN_LED_RED, LOW);
        delay(300);
    }
}