
# Firmware Take Home Test

## Objective

The objective of this test is to gain an idea of your understanding of basic firmware development principles
and let us get a feel for your individual coding style. You will use a circle trackpad development kit and write code that 
will run on the Arduino. The code you write for the Arduino communicates with a circle trackpad and adds the 
functionality shown below.

![Development Kit](CircleTrackpadDevKit_Small.jpg)

## The Test

The test can be broken up into the following tasks. 

1. Determine if the circle touchpads are I2C or SPI interface
2. Determine the overlay is curved or flat
3. Clone the sample code for the correct interface and get that code built and running
4. Modify the sample code to do the following:
    1. Imagine the sensor is divided into four quadrants
    2. When a touch appears in a quadrant, stays in that quadrant, and leaves within 300 msec you report a "tap in a quadrant" by printing to the serial port Qx. Where X is the quadrant number (1..4).
	3. Test cases:
	    1. The code has no compile warnings and no compile errors
        2. When nothing is touching the sensor, nothing should be reported
        3. If a finger touches a quadrant, stays in that quadrant, and lifts off within 300 msec the report happens at lift-off of the finger
        4. If the finger touches a quadrant and slides to another quadrant, then lifts off, nothing is reported
        5. If a finger touches a quadrant, stays touching for more than 300 msec, then lifts off, nothing is reported
        6. After a touch is complete and tap reporting is complete, test case ii applies
5. Send the code in for review

## Helpful Links

You can use any text editor you want, but the Arduino IDE will be able to build
and update firmware on the Teensy when you are ready to test. You will need to 
install Arduino IDE and the Teensyduino software to be able to build and run the
project. 

All the sample code uses libraries from Arduino and Teensyduino. If you are have questions about the objects being used, look for answers on the Arduino
and Teensy documentation and examples. 

* [Arduino IDE](https://www.arduino.cc/en/software)
* [Teensyduino](https://www.pjrc.com/teensy/td_download.html)
* [Demo Kit Git Repo](https://github.com/cirque-corp/Cirque_Pinnacle_1CA027)
* [Cirque Circle Trackpad Development Kit](https://www.cirque.com/circle-trackpad-dev-kit)
* [Teensy Serial object](https://www.pjrc.com/teensy/td_serial.html)

## Finishing the Test

Zip up your project and email it to Jon (jonb of cirque period com).
*email addresses are "some assembly required"
