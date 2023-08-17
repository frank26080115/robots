#include "RoachServo.h"
#include "RoachServo_defs.h"

enum
{
  SERVO_TOKEN = 0x76726553 // 'S' 'e' 'r' 'v'
};

static servo_t servos[MAX_SERVOS];              // static array of servo structures
static uint8_t ServoCount = 0;                  // the total number of attached servos

RoachServo::RoachServo()
{
  if (ServoCount < MAX_SERVOS) {
    this->servoIndex = ServoCount++;            // assign a servo index to this instance
  } else {
    this->servoIndex = INVALID_SERVO;           // too many servos
  }
  this->pwm = NULL;
}

RoachServo::RoachServo(int pin) : RoachServo()
{
  _pin = pin;
}

uint8_t RoachServo::begin()
{
  if (_pin >= 0) {
    return this->attach(_pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  }
  else {
    return INVALID_SERVO;
  }
}

uint8_t RoachServo::attach(int pin)
{
  return this->attach(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

uint8_t RoachServo::attach(int pin, int min, int max)
{
  if (this->servoIndex == INVALID_SERVO) {
    return INVALID_SERVO;
  }
  bool succeeded = false;

  if (min < MIN_PULSE_WIDTH)
  {
    min = MIN_PULSE_WIDTH;
  }

  if (max > MAX_PULSE_WIDTH)
  {
    max = MAX_PULSE_WIDTH;
  }

  //fix min if conversion to pulse cycle value is too low
  if ( (min / DUTY_CYCLE_RESOLUTION) * DUTY_CYCLE_RESOLUTION < min )
  {
    min += DUTY_CYCLE_RESOLUTION;
  }

  this->min = min;
  this->max = max;

  // Adafruit, add pin to 1 of available Hw PWM
  // first, use existing HWPWM modules (already owned by Servo)
  for ( int i = 0; i < HWPWM_MODULE_NUM; i++ )
  {
    if ( HwPWMx[i]->isOwner(SERVO_TOKEN) && HwPWMx[i]->addPin(pin) )
    {
      this->pwm = HwPWMx[i];
      succeeded = true;
      break;
    }
  }

  // if could not add to existing owned PWM modules, try to add to a new PWM module
  if ( !succeeded )
  {
    for ( int i = 0; i < HWPWM_MODULE_NUM; i++ )
    {
      if ( HwPWMx[i]->takeOwnership(SERVO_TOKEN) && HwPWMx[i]->addPin(pin) )
      {
        this->pwm = HwPWMx[i];
        succeeded = true;
        break;
      }
    }
  }

  if ( succeeded )
  {
    pinMode(pin, OUTPUT);
    servos[this->servoIndex].Pin.nbr = pin;
    servos[this->servoIndex].Pin.isActive = true;

    this->pwm->setMaxValue(MAXVALUE); // sets the period
    this->pwm->setClockDiv(CLOCKDIV); // sets the resolution

    _pin = pin;

    return this->servoIndex;
  }
  else
  {
    return INVALID_SERVO;
  }
}

void RoachServo::detach()
{
  if (this->servoIndex == INVALID_SERVO) {
    return;
  }

  uint8_t const pin = servos[this->servoIndex].Pin.nbr;
  servos[this->servoIndex].Pin.isActive = false;

  // remove pin from HW PWM
  HardwarePWM * pwm = this->pwm;
  this->pwm = nullptr;
  pwm->removePin(pin);
  if (pwm->usedChannelCount() == 0) {
    pwm->stop(); // disables peripheral so can release ownership
    pwm->releaseOwnership(SERVO_TOKEN);
  }
}

void RoachServo::write(int value)
{
    if (value < 0) {
        value = 0;
    } else if (value > 180) {
        value = 180;
    }
    value = map(value, 0, 180, this->min, this->max);

    writeMicroseconds(value);
}


void RoachServo::writeMicroseconds(int value)
{
    if (this->pwm) {
        uint8_t pin = servos[this->servoIndex].Pin.nbr;
        this->pwm->writePin(pin, value/DUTY_CYCLE_RESOLUTION);
        this->currentUsX100 = value * 100;
    }
}

int RoachServo::read() // return the value as degrees
{
    return map(readMicroseconds(), this->min, this->max, 0, 180);
}

int RoachServo::readMicroseconds()
{
    if (this->pwm) {
        //uint8_t pin = servos[this->servoIndex].Pin.nbr;
        //return this->pwm->readPin(pin)*DUTY_CYCLE_RESOLUTION;
        return (this->currentUsX100 + 50) / 100;
    }
    return 0;
}

bool RoachServo::attached()
{
    if (this->servoIndex == INVALID_SERVO) {
        return false;
    }
    return servos[this->servoIndex].Pin.isActive;
}

void RoachServo::rampTask()
{
    uint32_t now = millis();
    if ((now - rampTimestamp) < 10 || rampTarget < 0 || rampStepX100 < 0) {
        return;
    }

    uint32_t us = (currentUsX100 + 50) / 100;

    if (rampTarget > currentUsX100)
    {
        currentUsX100 += rampStepX100;
        us = (currentUsX100 + 50) / 100;
        us = us >= rampTarget ? rampTarget : us;

        // cannot use writeMicroseconds(x) for this, since that call will override currentUsX100
        uint8_t pin = servos[this->servoIndex].Pin.nbr;
        this->pwm->writePin(pin, us / DUTY_CYCLE_RESOLUTION);

        rampTimestamp = now;
        if (us >= rampTarget) {
            rampTarget = -1;
            currentUsX100 = us * 100;
        }
    }
}

void RoachServo::ramp(int32_t tgt, int32_t stepX100)
{
    uint32_t us = (currentUsX100 + 50) / 100;
    if (tgt <= us || stepX100 <= 0)
    {
        // no need to ramp
        writeMicroseconds(tgt);
        rampTarget = -1;
        rampStepX100 = -1;
        return;
    }
    rampTarget = tgt;
    rampStepX100 = stepX100;
    rampTask();
}
