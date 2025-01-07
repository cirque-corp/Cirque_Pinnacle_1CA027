
# Firmware Take Home Test

## Objective

The objective of this test is to gain an idea of your understanding of basic firmware development principles and let us get a feel for your individual coding style. You will use a circle trackpad development kit and write code that will run on the Arduino. The code you write for the Arduino communicates with a circle trackpad and meets the test requirements listed below.

![Development Kit](CircleTrackpadDevKit_Small.jpg)

## The Test

### Tasks

1. Clone the sample code for the correct interface and get that code built and running: [TakeHomeTest.ino](TakeHomeTest.ino)
2. Modify the sample code to meet these specifications:
    1. Imagine the sensor is divided into four quadrants, numbered (0..3)
    2. When a finger touches a quadrant, stays in that quadrant, and lifts off within 300 msec, you log a tap by printing to the serial port the string "Qx\n",  where 'x' is the quadrant number
3. [Email your final code to us for review](#finishing-the-test)

### Test Requirements 
1. The code has no compile warnings and no compile errors
2. When nothing is touching the sensor, nothing should be reported
3. A report string is sent if and only if:
    1. A finger touches a quadrant
    2. The finger stays in that quadrant
    3. The finger lifts off within 300 msec
4. If the finger touches a quadrant and slides to another quadrant, then lifts off, nothing is reported
5. If a finger touches a quadrant, stays touching for more than 300 msec, then lifts off, nothing is reported

### Examples

1. A finger touches the second quadrant and lifts off 100ms later
    1. The code reports "Q1\n"
2. A finger touches the fourth quadrant and stays touching for 500ms, and then lifts off
    1. The code reports nothing (touch longer than 300ms)
3. A finger touches the first quadrant and slides to quadrant 2, and lifts off in 100ms
    1. The code reports nothing (touch moved quadrants)

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
