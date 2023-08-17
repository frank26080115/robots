Project Roach - List of Libraries
=================================

  * AsyncAdc
    * non-blocking implementation for reading ADC
    * polling
    * full implementation for both ESP32 and nRF52
    * used by RoachPot
  * NonBlockingTwi
    * non-blocking implementation of I2C master
    * for nRF52 only
    * uses a linked list to queue transactions
    * uses DMA for transactions
  * nRF5Rand
    * true random number generator (unknown specifications, assumed to be secure enough for Bluetooth)
    * keeps a queue of random numbers that are automatically periodically generated
    * intelligently switches between interrupt mode and polling mode
    * used by nRF52RcRadio, not used frequently, so polling mode is preferred
  * nRF52Dshot
    * implements DSHOT protocol for brushless ESCs
    * uses a PWM generator and DMA to generate pulses
    * some API similarities to Servo library for compatibility
  * nRF52OneWireSerial
    * allows for the nRF52 MCU to become a serial link to brushless ESCs running AM32 or BLHeli_32
    * this is used for reconfiguring brushless ESCs, or even updating their firmware
    * there's a bit-bang implementation (which is blocking), and also a hardware UART implementation (which is non-blocking)
  * nRF52RcRadio
    * this is the most important part of the Roach project
    * implements the robust, secure, low-latency radio link
    * [click here to read more](../doc-nrf52-radio-protocol.md)
  * nRF52UicrWriter
    * used to write data into UICR, mainly used to disable NFC on the nRF52840
  * RoachButton
    * button reading library
    * handles debouncing, button holding
    * both polling and interrupt
  * RoachCmdLine
    * implements a serial port command line interface
    * executes functions according to user input through serial terminal
    * used for debugging and executing test code
  * RoachDriveMixer
    * mixes "arcade" drive commands into "tank" drive values for a left and right motor
    * handles application of calibration and other adjustable parameters
    * also mixes in gyro correction if needed
  * RoachHeadingManager
    * calculates both current heading angle and target heading angle, using user input and IMU data
    * manages manual steering input override (without this library, manual override would be impossible)
  * RoachHeartbeat
    * there's two functions in here to implement a watchdog timer
    * there is a LED blink engine that can take "animations" defined by an array
    * classes to control discrete RGB LEDs, NeoPixels, and DotStars
    * NeoPixel implementation uses PWM generator and DMA
    * DotStar implementation uses SPI and DMA
    * discrete RGB LED uses PWM generator
  * RoachIMU
    * asynchronously reads IMU
    * simply outputs roll, pitch, and yaw (heading angle)
    * handles checking if the robot is upside-down
    * BNO085 implementation uses its internal fusion algorithm, and "CEVA sensor hub"
    * LSM6DS3 implementation uses Mahony AHRS algorithm
  * RoachLib
    * functions that are common to both the transmitter and receiver
    * such as handling configuration files
    * such as synchronizing configuration files from receiver to transmitter
    * such as unifying math functions to make sure units are always correct
    * defines some common data structures, such as radio payload structure
  * RoachPerfCnt
    * performance counter for debugging/profiling
    * simply counts the number of times the task executes in one second
    * if the count is too low, below 10ms per count, then the system will malfunction
  * RoachPID
    * implements a PID controller
    * for angles, so wrap-around is handled gracefully
    * internally used units are weird
    * D term polarity is handled automatically
  * RoachPot
    * analog potentiometer reading
    * handles calibration of center and end-points
    * handles dead-zone and other configurable parameters
    * handles digital LPF filtering
    * uses AsyncAdc, non-blocking
    * if multiple instances are used, they are polled in round-robin fashion
  * RoachRobot
    * convenience functions that are common to all robots (receiver) that can be built with Project Roach
  * RoachRobotNvm
    * convenience functions for handling configuration files, that are common to all robots (receiver) that can be built with Project Roach
  * RoachRotaryEncoder
    * handles reading of quadrature encoder
    * has both an implement using plain GPIO interrupts, and another implementation using a dedicated encoder peripheral
  * RoachServo
    * a copy of the original Servo library, but with higher resolution and some other tweaks
  * RoachUsbMsd
    * When a MCU using Project Roach is connected by USB, it defaults to enumerating as only a CDC serial port.
    * When RoachUsbMsd is told to, it will re-enumerate as a USB drive (and also CDC serial port)
    * Firmware code cannot read or write to files on the flash memory while the USB drive mode is active, so it is critical that USB drive mode can be activated or deactivated by the user at any time
  * Adafruit_SSD1306
    * this is a modified version of the original Adafruit_SSD1306 library
    * this uses the NonBlockingTwi library internally
    * meaning the OLED screen updates are happening asynchronously
    * typical full screen refresh takes 23ms, this modified library makes sure this time is not wasted
    * the radio must transmit two messages during this 23ms!
  * Adafruit_SPIFlash
    * this is a modified version of the original Adafruit_SPIFlash library
    * added support for the flash IC P25Q16H, which is used on the Seeeduino XIAO BLE
