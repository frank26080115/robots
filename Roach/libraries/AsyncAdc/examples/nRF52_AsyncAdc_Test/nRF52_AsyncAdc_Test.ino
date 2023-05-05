#include <AsyncAdc.h>
#include <Adafruit_TinyUSB.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  analogCalibrateOffset();
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t t = 0;
  uint32_t now = millis();
  if ((now - t) >= 1000) {
    Serial.print(now);
    adcStart(A0);
    while (adcBusy(A0)) {
      Serial.print(".");
    }
    Serial.println(adcEnd(A0));
    t = now;
  }
}
