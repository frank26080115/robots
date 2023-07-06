#include <Adafruit_TinyUSB.h>
#include <nRF52OneWireSerial.h>

#define SERVO_PIN A1

nRF52OneWireSerial
    #ifdef NRFOWS_USE_HW_SERIAL
        ow(&Serial1, NRF_UARTE0, SERVO_PIN);
    #else
        ow(SERVO_PIN, 19200, false);
    #endif

void setup()
{
    sd_softdevice_disable();
    pinMode(7, INPUT_PULLUP);
    Serial1.begin(19200);
    Serial.begin(19200);
    ow.begin();
}

void loop()
{
    int i, j;
    while ((i = Serial.available()) > 0)
    {
        for (j = 0; j < i; j++)
        {
            char c = Serial.read();
            ow.write(c);
            Serial.write(c);
        }
    }
    while ((i = ow.available()) > 0)
    {
        for (j = 0; j < i; j++)
        {
            char c = ow.read();
            Serial.write(c);
        }
    }
}
