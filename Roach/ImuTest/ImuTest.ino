#include <RoachIMU.h>
#include <NonBlockingTwi.h>
#include <Adafruit_TinyUSB.h>

#define ROACHHW_PIN_I2C_SCL 23
#define ROACHHW_PIN_I2C_SDA 22

RoachIMU imu(5000, 0, ROACHIMU_ORIENTATION_XZY | ROACHIMU_ORIENTATION_FLIP_ROLL, BNO08x_I2CADDR_DEFAULT, -1);

void setup()
{
  Serial.begin(115200);
  nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE);
  imu.begin();
}

void loop()
{
  static uint32_t last_time = 0;
  static uint32_t last_cnt = 0;
  static uint32_t drate = 0;
  uint32_t now = millis();
  nbtwi_task();
  imu.task();
  if (imu.has_new)
  {
    imu.doMath();
    imu.has_new = false;
    Serial.printf("%0.1f   %0.1f   %0.1f", imu.euler.yaw, imu.euler.pitch, imu.euler.roll);
    if (imu.is_inverted)
    {
      Serial.printf("   INV %0.1f", imu.heading);
    }
    if (imu.totalFails() > 0) {
      Serial.printf("   FAILS %u", imu.totalFails());
    }
    Serial.printf("   DRATE %u", drate);
    Serial.printf("\r\n");
  }

  if ((now - last_time) >= 1000)
  {
    last_time = now;
    int x = imu.getTotal();
    int y = x - last_cnt;
    last_cnt = x;
    drate = y;
  }

  if (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == 'z')
    {
      imu.tare();
      Serial.printf("tare\r\n");
    }
  }
}
