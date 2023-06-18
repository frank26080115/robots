#ifndef _ROACHSERVO_DEFS_H_
#define _ROACHSERVO_DEFS_H_

#include <Arduino.h>

//#define DUTY_CYCLE_RESOLUTION   8
//#define MAXVALUE                2500 // 20ms
//#define CLOCKDIV                PWM_PRESCALER_PRESCALER_DIV_128

#define DUTY_CYCLE_RESOLUTION   1
#define MAXVALUE                10000 // 10ms
#define CLOCKDIV                PWM_PRESCALER_PRESCALER_DIV_16

// define one timer in order to have MAX_SERVOS = 12
typedef enum { _timer1, _Nbr_16timers } timer16_Sequence_t;

#define Servo_VERSION           2     // software version of this library

#define MIN_PULSE_WIDTH       544     // the shortest pulse sent to a servo
#define MAX_PULSE_WIDTH      2400     // the longest pulse sent to a servo
#define DEFAULT_PULSE_WIDTH  1500     // default pulse width when servo is attached
#define REFRESH_INTERVAL    20000     // minumim time to refresh servos in microseconds 

#define SERVOS_PER_TIMER       12     // the maximum number of servos controlled by one timer 
#define MAX_SERVOS             (_Nbr_16timers  * SERVOS_PER_TIMER)

#define INVALID_SERVO         255     // flag indicating an invalid servo index

typedef struct  {
    uint8_t nbr        :6 ;             // a pin number from 0 to 63
    uint8_t isActive   :1 ;             // true if this channel is enabled, pin not pulsed if false 
} ServoPin_t   ;  

typedef struct {
    ServoPin_t Pin;
    volatile unsigned int ticks;
} servo_t;

#endif
