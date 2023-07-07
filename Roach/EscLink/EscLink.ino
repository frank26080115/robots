#include <Adafruit_TinyUSB.h>
#include <nRF52OneWireSerial.h>

#define SERVO_PIN A1

nRF52OneWireSerial
    #ifdef NRFOWS_USE_HW_SERIAL
        ow(&Serial1, NRF_UARTE0, SERVO_PIN);
    #else
        ow(SERVO_PIN, 19200, false);
    #endif

#ifdef ESCLINK_TESTGEN
Uart Serial2 = Uart(NRF_UARTE1, UARTE1_IRQn, A3, A2);
#endif

void setup()
{
    sd_softdevice_disable();
    Serial1.begin(19200);
    Serial.begin(19200);
    #ifdef ESCLINK_TESTGEN
    Serial2.begin(19200);
    #endif
    ow.begin();
    #ifdef ESCLINK_TESTGEN
    ow.println("hello");
    Serial2.println("welcome");
    Serial2.flush();
    #endif
}

void loop()
{
    ow.echo(&Serial);
    #ifdef ESCLINK_TESTGEN
    static uint8_t i = 0;
    Serial2.write((uint8_t)i);
    Serial2.flush();
    i++;
    #endif
}

#ifdef ESCLINK_TESTGEN
extern "C"
{
  void UARTE1_IRQHandler()
  {
    Serial2.IrqHandler();
  }
}
#endif
