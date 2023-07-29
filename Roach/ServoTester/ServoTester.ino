#include <Adafruit_TinyUSB.h>
#include <RoachPot.h>
#include <RoachButton.h>
#include <RoachServo.h>

RoachButton btn1   = RoachButton(D1);
RoachButton btn2   = RoachButton(D2);
RoachPot    pot    = RoachPot(A0, NULL);
RoachServo  drive  = RoachServo(D3);
nRF52Dshot  weapon = nRF52Dshot(D4);

void setup()
{
    Serial.begin(115200);
    pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
    pinMode(ROACHHW_PIN_LED_BLU, OUTPUT);
    digitalWrite(ROACHHW_PIN_LED_RED, HIGH);
    digitalWrite(ROACHHW_PIN_LED_BLU, LOW);
    RoachButton_allBegin();
    RoachPot_allBegin();
    drive.begin();
    drive.writeMicroseconds(1500);
    weapon.begin();
    weapon.writeMicroseconds(1000);
}

void loop()
{

}
