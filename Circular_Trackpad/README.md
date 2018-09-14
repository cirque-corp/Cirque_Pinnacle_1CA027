Circular Touchpads

The circular touch pad demo-code was designed and tested using the Development Kit
available by following the link https://www.cirque.com/circle-trackpad-dev-kit.
Sample code is compatible with TM040040, TM035035 and TM023023 touchpads (touch
module part number is located above the connector on pad).

If you are just getting started, the simplest example to learn and understand is
the Single_Pad_Sample_Code. The programs in this directory fetch touch data from
the single touchpad and use the Arduino Serial Monitor to display the results.

The programs located in the Dual_Pad_Sample_Code are similar to the single pad
programs, but will display touch data from both touchpads.

Hardware Controlled Options:

Depending on the application, the pinnacle device can be configured to communicate
via SPI or I2C. IF YOUR DEVICE IS NOT CONFIGURED FOR THE PROTOCOL YOU ARE USING
IT WILL NOT WORK. For SPI, ensure the resistor R1 on the back of the sensor is
populated. Use a 0402 470K resistor in R1 to enable SPI communication. If the
intended operation uses I2C before running the program. 
