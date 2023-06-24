#include <RoachIMU.h>
#include <NonBlockingTwi.h>
#include <Adafruit_TinyUSB.h>

#define ROACHHW_PIN_I2C_SCL 23
#define ROACHHW_PIN_I2C_SDA 22

RoachIMU imu(10000, 0, 0, BNO08x_I2CADDR_DEFAULT, -1);

void setup() {
  Serial.begin(115200);
  while (true)
  {
    Serial.printf("hello\r\n");
    delay(500);
    if (Serial.available())
    {
        Serial.read();
        break;
    }
  }
  nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE);
  imu.begin();
}

void loop() {
  nbtwi_task();
  imu.task();
  if (imu.has_new) {
    imu.doMath();
    imu.has_new = false;
    Serial.printf("%f\r\n", imu.heading);
  }
}
