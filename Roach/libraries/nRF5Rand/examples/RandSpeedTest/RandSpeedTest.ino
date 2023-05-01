#include <Adafruit_TinyUSB.h>
#include <nRF5Rand.h>

void setup() {
  Serial.begin(115200);
  nrf5rand_init(1024 * 8);
}

void loop() {
  static uint32_t t = 0;
  static uint32_t c = 0;
  uint32_t now = millis();
  c += nrf5rand_avail();
  nrf5rand_flush();
  if ((now - t) >= 1000)
  {
    Serial.printf("%u, %u\r\n", now, c);
    c = 0;
    t = now;
  }
}
