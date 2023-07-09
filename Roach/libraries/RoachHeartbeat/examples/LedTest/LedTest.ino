#include <RoachHeartbeat.h>
#include <Adafruit_TinyUSB.h>

RoachHeartbeat hb = RoachHeartbeat(RHB_HW_PIN_ITSYBITSY_LED);

const uint8_t blink_pattern[] = { 0x42, 0x23, 0x2A, 0x00};
const uint8_t blink_pattern2[] = { 0x11, 0x11, 0x11, 0x00};

//RoachRgbLed rgb = RoachRgbLed(true);
RoachNeoPixel rgb = RoachNeoPixel();

void setup()
{
    Serial.begin(115200);

    //while (Serial.available() <= 0) {
    //    yield();
    //}

    Serial.println("hello LED test");

    hb.begin();
    rgb.begin();
    //rgb.set(0xFF, 0, 0xFF);

    hb.play(blink_pattern);
    hb.task();
    hb.queue(blink_pattern2);
}

void loop()
{
    for (int i = 0; i <= 65535; i += 16) {
        rgb.setHue(i, 8);
        hb.task();
        delay(1);
    }
}
