#include <nRF52Dshot.h>
#include <Adafruit_TinyUSB.h>

nRF52Dshot esc = nRF52Dshot(5);

void setup()
{
    Serial.begin(19200);
    esc.begin();
}

void loop()
{
    static uint16_t throttle = 1000;
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if ((now - last_time) >= 10)
    {
        throttle += 10;
        if (throttle > 2000) {
            throttle = 1000;
        }
        esc.setThrottle(nrfdshot_convertPpm(throttle));
    }
    esc.task();
}
