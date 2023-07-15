#include <RoachHeartbeat.h>
#include <Adafruit_TinyUSB.h>

void setup()
{
    Serial.begin(115200);
    while (Serial.available() <= 0) {
        yield();
    }
    Serial.println("hello wdt test");
    RoachWdt_init(500);
}

int dly = 100;

void loop()
{
    Serial.printf("testing %u\r\n", dly);
    for (int i = 0; i < 10; i++)
    {
        delay(dly);
        RoachWdt_feed();
    }
    dly += 100;
}
