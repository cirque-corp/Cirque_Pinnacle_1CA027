# Pinnacle Command Panel Demo
Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

### Overview:
This is a sample Arduino program that displays sensor data from track pad
to the Serial Monitor. It is an advanced version of the simple TM040040/TM035035
demo that abstracts all the Pinnacle features into simple functions that can be
from a basic text controlled menu on the serial monitor and can be used in other
embedded applications (see Pinnacle.h and Pinnacle.c).

### Menu:
The program outputs a simple menu to the serial monitor at the start up of the
program. Entering the associated character will activate or disable the listed feature. For more information, see the descriptions below.

**a - set to absolute mode**
    Selecting this will put the touchpad in absolute mode in which the device
    will report the absolute location of the detected movement.

**c - force sensor to recalibrate**
    This option triggers the sensors recalibration routine. Useful when Touchpad
    is missing real touch events.

**d - disable the feed**
    This function mutes the feed from the sensor.

**e - enable the feed**
    This function enables the feed from the sensor.

**f - enable curved overlay**
    This command enables curved overlay compensation for the given sensor. If
    the sensor is flat it will increase the gain of the sensor at the edges and
    register touch events before the finger contacts the touchpad. The device
    must be collecting data in absolute mode for this feature to be activated.
    By default, this features is disabled.

**g - disable curved overlay**
    Disabling the curved overlay removes curved overlay compensation.

**m - get comp-matrix data**
    Cirque devices using the Pinnacle ASIC use a compensation matrix under the
    hood to tune the device to the current environment. Selecting this menu
    option will return the current compensation matrix.

**r - set to relative mode**
    This selection will put the sensor in relative mode, which dynamically sets
    each touchdown point as the origin and reports the coordinates relative to
    the touchdown point.

**s - toggle enable/disable sensor**
    This function toggles which sensor output is displayed to the monitor.

**l - list these commands again**
    This simply lists the available commands in the menu.

### Example Output from Serial Monitor:

'   Commands:
    a - set to absolute mode
    c - force sensor to recalibrate
    d - disable the feed
    e - enable the feed
    f - enable curved overlay
    g - disable curved overlay
    m - get comp-matrix data
    r - set to relative mode
    s - toggle enable/disable sensor
    l - list these commands again

    SENS_0 1141	500	63		SENS_1 2045	497	27	0
    SENS_0 1150	485	63		SENS_1 2045	494	29	0
    SENS_0 1161	474	63		SENS_1 2045	490	28	0
    SENS_0 1173	466	63		SENS_1 2045	485	29	0
    SENS_0 1186	462	63		SENS_1 2045	482	26	0
    SENS_0 1203	460	63		SENS_1 2045	481	26	0
    SENS_0 1225	460	63		SENS_1 2026	482	26	0
    SENS_0 1253	463	63		SENS_1 1935	491	27	0
    SENS_0 1309	480	63		SENS_1 1812	524	23	0
    SENS_0 1363	502	63		SENS_1 1745	571	19	0
    SENS_0 1414	529	63		SENS_1 1711	595	16	0
    SENS_0 1460	561	63		SENS_1 1684	603	14	0
    SENS_0 1500	598	62		SENS_1 1680	611	13	0
    SENS_0 1533	637	63		SENS_1 1698	627	12	0
    SENS_0 1562	674	63		SENS_1 1736	658	10	0'

### Troubleshooting:

1. Ensure the device is connected to the computer via the USB cable provided. If
the device is connected, try disconnecting, waiting 10 seconds and reconnecting
the device before uploading in the Arduino IDE.

2. Check the cables between the Development Board and the Pinnacle sensor.
Inspect the cable for damage and reconnect device.
