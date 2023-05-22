Handheld Pistol Grip Style Remote Controller
============================================

This style of remote controller is much better suited for driving wheeled vehicles, as opposed to stick style which is better for aircraft.

The index finger controls the throttle through a trigger mechanism. The right hand rotates the steering wheel to turn the robot.

This project is entirely 3D printed. The transparent cover is laser cut but it could have been 3D printed as well. It is designed in 5 major pieces:

 * bottom heel, contains the quadrature encoder for the bottom wheel, contains the battery, acts as a heel for the remote to sit on.
 * top front, contains the trigger and steering mechanisms, including the spring centering mechanisms
 * top rear, an electronics enclosure
 * pistol grip, this is a thick chunk of plastic that all the other chunks get screwed on to
 * cover plate, transparent to show off the spring tensioning mechanisms, laser cut or 3D printed

The two spring centering mechanisms use a lever arm to press against a flat portion on the trigger and steering wheel. Tension springs are used to apply the spring tension. The trigger is dimensioned so that the force required to pull the trigger is larger than the force required to push it. The steering wheel is dimensioned so that the force is equal between the left and right, even though there is only one level.

Very basic 10 kilo-ohm potentiometers are used as the sensors for the trigger and the steering wheel, they are gigantic so they should last quite a long time, they are also cheap and easy to replace. An optical quadrature encoder with 400 pulse-per-revolution is used for the heading input wheel, it should be extremely durable and have very good resolution.

All the triggers and steering wheels are attached to D-shafts, and screws are used to secure them. For durability, a rectangular nut is embedded into the plastic so that the screw threads are strong and allow for the screws to be very tight.

The electronics enclosure houses two circuit boards on the inside, and the OLED display is attached on the outside. There are also several holes where panel-mounted potentiometers and switches are screwed into. The design isn't very optimal, it's very large, making the whole remote top-heavy and towards the left, but I wanted room to make electrical work easy. The heel of the remote is enlarged to compensate for the imbalance.

Power
-----

The whole remote is powered by a single cell lithium battery, and is recharged by the Adafruit Feather when a USB cable is connected.

There is a 5V boost converter in the circuit, for two reasons:

 * the optical quadrature encoder recommends powering with 5V
 * if potentiometers are powered from the 3.3V LDO, their readings will dip if the battery goes below 3.5V, by powering the potentiometers from a steady higher voltage, their output will never dip because of low battery

note: the nRF52840 uses an internal 0.6V reference for the ADC, and the ADC inputs have an internal PGA that can perform a 1/6 gain on the input

The battery cell is a 18650 cylindar cell, the heel of the remote is designed to house a 18650 cell holder.

Graphical User Interface
------------------------

I used a monochrome OLED display with 128 x 64 pixels. It is able to display many lines of text with a small font. The circuit board with the display also has many directional and tactile buttons. The firmware of the nRF52840 uses all of this to implment a GUI with a menu that allows me to select parameters from a list, and then choose to adjust that parameter. Also I can load from or save to a list of files on the flash memory.

The OLED is using I2C to communicate, which is quite slow, even at 400 KHz bus speed, it would've taken way over 20 milliseconds to update the entire screen. This wouldn't work with the requirements of sending off a radio packet every 10 milliseconds. The solution was to edit Adafruit's graphics library to place all I2C transactions in a queue. The queue is a FIFO implemented as a linked list. The queued I2C transactions are only sent in the background when the radio code isn't busy, luckily the nRF's internal I2C hardware is able to send entire buffers without blocking the code. A state machine handles all of this. Thus, the radio code is able to execute whenever it wants even if I2C packets are also being transmitted.

In fact, literally nothing in my code is blocking, there are literally no waits or delays, everything is using either interrupts or state machines. Even the ADC is sampled continuously without blocking the code. The IMU on the robot is using a similar techique as well.

(note: I prefer using state machines instead of threads, even though technically the nRF52840 has FreeRTOS, it only has a single core, so I don't see the benefits of threads outweighing the downsides)
