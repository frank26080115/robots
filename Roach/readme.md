Project - Roach
===============

Roach is a project of electronics and firmware for competitive combat robots. It is an experimental platform for me to try adding IMU sensors to the robot so it can accomplish things like absolute heading control, traction control, and perhaps even utilizing its combat weapon as a reaction wheel. Why?

 * When you are aimed at your opponent, you need to make sure inputting full throttle makes the robot go exactly straight. The robots often have heavy spinning weapons that have a gyroscopic effect and yaw axis torque, which makes precise driving difficult. Hence I feel like adding the IMU to assist the drivetrain could be a competitive advantage.

 * When either yours or your oppnents weapon actually impacts, your robot will spin violently, an IMU can assist in recovering from the spin and maintain an offensive or defensive stance quicker. (weapon towards opponent, or armour towards opponent)

 * When the robot loses traction, a spinning weapon can be used like a reaction wheel to assist in maintaining an offensive or defensive stance. (this is debatable if it is useful as it is trading off weapon energy momentarily)

These combat robots typically use the same kind of RC remote controllers as model aircraft. To attempt my experiments, there are these factors that make it neccessary to ditch traditional RC remote controllers:

 * the robot needs to have a microcontroller that can communicate with a IMU sensor, and perform feedback loop control
 * the remote transmitter needs to be able to send data in a format that is ideal for representing a heading angle
 * the remote transmitter needs to have an quadrature encoder to take heading angle input from the user

The Roach project accomplishes this using circuitry based around the nRF52840 microcontroller, utilizing its integrated 2.4 GHz radio for data transmission. The board I am using is the Adafruit Feather nRF52840 Express and Adafruit ItsyBitsy nRF52840, both of these products include a flash memory IC.

 * The radio allows for low level controls, allowing me to implement a protocol that is low latency, highly reliable, and less susceptible to interference.
 * The flash memory IC is combined with the nRF52840's native USB device port, so that it can become a USB flash drive. Since this project will involve things like PID control loops, it is very nice to be able to edit the PID parameters easily with a text editor, save backups, and be able to load multiple configurations during competitions.

The IMU I started the project with is the BNO085, which features its own internal processing, so it can output the robot's Euler angles without much calculations by the nRF52840. It is rated for 2000 G of mechanical shock over 200 microseconds, which I calculated to be enough to handle most hits taken during small insect class combat robot fights.

The remote controller is built with 3D printing, in the style of a pistol grip RC remote, which means a trigger for controlling throttle, and a wheel for steering. The steering wheel is spring-loaded and uses a potentiometer as the sensor. I added an additional wheel on the bottom that is not spring-loaded and uses a optical quadrature encoder as the sensor, to provide the heading angle input. There are some other potentiometers and switches around the remote to implement other inputs, such as adjusting weapon speed. On top of the radio is a small OLED display so that I can implement a GUI, along with several tactile and directional buttons to operate the GUI.

The engineering behind this project involved many smaller milestones that I am quite proud of. If you want to learn more, please continue reading these topics:

 * The robot: Detcord
 * Using the nRF52840 radio to implement a high performance and secure data link