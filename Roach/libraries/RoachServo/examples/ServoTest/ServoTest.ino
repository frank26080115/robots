#include <Adafruit_TinyUSB.h>
#include <RoachServo.h>

RoachServo servo1(A1);
RoachServo servo2(6);
RoachServo servo3(9);

void setup()
{
    Serial.begin(115200);
    //servo1.attach(5);
    //servo2.attach(6);
    //servo3.attach(9);
    servo1.begin();
    servo2.begin();
    servo3.begin();
    while (Serial.available() <= 0) {
        yield();
    }
}

void loop()
{
    int i;
    for (i = 1000; i < 2000; i += 100)
    {
        servo1.writeMicroseconds(i);
        servo2.writeMicroseconds(i + 1);
        servo3.writeMicroseconds(i + 2);
        delay(100);
    }
    for (i = 2000; i >= 1000; i -= 100)
    {
        servo1.writeMicroseconds(i);
        servo2.writeMicroseconds(i);
        servo3.writeMicroseconds(i);
        delay(30);
    }
    servo1.writeMicroseconds(0);
    servo2.writeMicroseconds(0);
    servo3.writeMicroseconds(0);
    delay(1000);
}
