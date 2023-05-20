#include <Adafruit_TinyUSB.h>
#include <nRF5Rand.h>

void setup() {
  Serial.begin(115200);
  Serial.println("hello world");
  nrf5rand_init(1024 * 8, true, true);
}

void loop() {
  Serial.printf("%d, %d, %d\r\n", millis(), nrf5rand_avail(), nrf5rand_u32());
  delay(100);
}
