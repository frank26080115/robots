#ifndef _ROACHSERVO_H_
#define _ROACHSERVO_H_

#include <Arduino.h>

class RoachServo
{
    public:
        RoachServo();
        RoachServo(int pin);                       // alternative constructor for pin assignment
        uint8_t begin();                           // attach the pin given from constructor
        uint8_t attach(int pin);                   // attach the given pin to the next free channel, sets pinMode, returns channel number or INVALID_SERVO if failure (zero is a valid channel number)
        uint8_t attach(int pin, int min, int max); // as above but also sets min and max values for writes. 
        void detach();
        void write(int value);                     // if value is < 200 its treated as an angle, otherwise as pulse width in microseconds 
        void writeMicroseconds(int value);         // Write pulse width in microseconds 
        int read();                                // returns current pulse width as an angle between 0 and 180 degrees
        int readMicroseconds();                    // returns current pulse width in microseconds for this servo (was read_us() in first release)
        bool attached();                           // return true if this servo is attached, otherwise false 
    private:
        int _pin = -1;
        uint8_t servoIndex;                        // index into the channel data for this servo
        int16_t min;                               // minimum pulse in µs
        int16_t max;                               // maximum pulse in µs

        HardwarePWM* pwm;
};

#endif
