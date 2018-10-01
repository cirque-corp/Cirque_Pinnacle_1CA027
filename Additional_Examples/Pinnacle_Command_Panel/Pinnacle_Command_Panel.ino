// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Hardware.h"
#include "Pinnacle.h"
#include <string.h>

// ___ Interfacing to Touchpads Based on Cirque's Pinnacle (1CA027) ASIC ___
// This demonstration application is built to work with a Teensy 3.2 but can easily be adapted to
// work with Arduino-based systems.
// This application connects to a touchpad via SPI. Use Tools->Serial Monitor to interact with the demo.

//  NOTE: <internal> Consider implementing interupts to manage the output with dashes for null input.

//  Pinnacle TM040040 (or other Pinnacle-based circle sensors) with Arduino
//  Required Connections:
//    GND
//    +3.3V
//    SCK = Pin 13
//    MISO = Pin 12
//    MOSI = Pin 11
//    SS = PIN 10
//    DR = Pin 9

#define LED0_PIN 21
#define LED1_PIN 20
#define SENSOR_0 0
#define SENSOR_1 1

// Struct to manage unique sensor attributes.
typedef struct _senFlag
{
  bool senSel;
  touchData_t touchData;
} senFlag_t;

senFlag_t senData[2];



// setup() gets called once at power-up, sets up serial debug output and Cirque's Pinnacle ASIC.
void setup()
{
  Serial.begin(115200);
  while(!Serial);
  delay(750);   // Wait for USB port to enumerate

  senFlag_t tempFlag;
  touchData_t tempTouch;

  tempFlag.senSel = true;
  tempFlag.touchData = tempTouch;

  senData[0] = tempFlag;
  senData[1] = tempFlag;

  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);

    // Initialize Hardware variables and SPI communication before initializing Pinnacle
  HW_init();
  SPI_init(1000000, MSBFIRST, SPI_MODE1);
  delay(100);
  cyclePower();   // Cycle the power on Pinnacle to refresh registers.

  Pinnacle_init(&senData[SENSOR_0].touchData, SENSOR_0);
  Pinnacle_init(&senData[SENSOR_1].touchData, SENSOR_1);

  printInstructions();
}

// loop() continuously checks to see if data-ready (DR) is high. If so, reads and reports touch data to terminal.
void loop()
{
  uint8_t rxByte, i;
  uint8_t sensorId = 0;
  int16_t compData[46];
  String printData = "";

  // Fetch and format touch data for display for both sensors.
  if(Pinnacle_available(SENSOR_0) && senData[SENSOR_0].senSel)
  {
    Pinnacle_getTouchData(&senData[SENSOR_0].touchData, SENSOR_0);
    printData += "SENS_0 ";
    toStringTouchData(senData[SENSOR_0].touchData, &printData);
    digitalWrite(LED0_PIN, LOW);
  }
  else
  {
    digitalWrite(LED0_PIN, HIGH);
  }

  if(Pinnacle_available(SENSOR_1) && senData[SENSOR_1].senSel)
  {
    Pinnacle_getTouchData(&senData[SENSOR_1].touchData, SENSOR_1);
    printData += (senData[SENSOR_1].senSel && printData.length() == 0) ? "\t\t\t\t\tSENS_1 " :
      (senData[SENSOR_0].senSel) ? "\tSENS_1 " :
      "SENS_1 ";
    toStringTouchData(senData[SENSOR_1].touchData, &printData);
    digitalWrite(LED1_PIN, LOW);
  }
  else
  {
    digitalWrite(LED1_PIN, HIGH);
  }

  // If the there is touch data to display, push it to the serial monitor
  if (printData.length() != 0)
  {
    // append button data to display string (4 = Button 1, 2 = Button 3, 1 = Button 2)
    printData.concat(String((senData[SENSOR_0].touchData.mode == ABSOLUTE) ? senData[SENSOR_0].touchData.absolute.buttons : senData[SENSOR_0].touchData.relative.buttons));
    Serial.println(printData);
  }

  if(Serial.available())
  {
    rxByte = Serial.read();
    if (rxByte != 'l') sensorId = getSensorSelect();  // Select sensor of action

    if (sensorId == 0xFF)
    {
      Serial.println("ERROR: Invalid sensor...");
    }
    else
    {

      switch(rxByte)
      {
        case 'a':
          Pinnacle_setToAbsolute(&senData[sensorId].touchData, sensorId);
          Serial.println("Set to absolute-mode...");
          Serial.println("X\tY\tZ\tButtons");
          break;
        case 'c':
          Serial.println("Forcing a calibration...");
          Pinnacle_forceCalibration(sensorId);
          Pinnacle_enableFeed(true, sensorId);      // Reenable feed after calibration
          Serial.println("Calibration complete...");
          break;
        case 'd':
          Pinnacle_enableFeed(false, sensorId);
          Serial.println("Feed disabled...");
          break;
        case 'e':
          Pinnacle_enableFeed(true, sensorId);
          Serial.println("Feed enabled...");
          break;
        case 'f':
          Pinnacle_enableCurved(&senData[sensorId].touchData, true, sensorId);
          Pinnacle_forceCalibration(sensorId);      // Adjust comp matrix after changing ADC data
          Pinnacle_enableFeed(true, sensorId);      // Reenable feed after calibration
          Serial.println("Curved Mode enabled...");
          break;
        case 'g':
          Pinnacle_enableCurved(&senData[sensorId].touchData, false, sensorId);
          Pinnacle_forceCalibration(sensorId);      // Adjust comp matrix after changing ADC data
          Pinnacle_enableFeed(true, sensorId);      // Reenable feed after calibration
          Serial.println("Curved Mode disabled...");
          break;
        case 'm':
          Serial.println("Reading comp-matrix...");
          Pinnacle_getCompMatrix(compData, sensorId);
          Pinnacle_enableFeed(true, sensorId);      // Reenable feed after fetching
          Serial.println("Comp-matrix values:");
          for(i = 0; i < 46; i++)
          {
            Serial.println(compData[i], DEC);
          }
          break;
        case 'l':
          printInstructions();
          break;
        case 'r':
          Pinnacle_setToRelative(&senData[sensorId].touchData, sensorId);
          Serial.println("Set to relative-mode...");
          Serial.println("xDelta\tyDelta\twheel\tbuttons");
          break;
        case 's':
          senData[sensorId].senSel = !senData[sensorId].senSel;
          Serial.println(Sensor_toString(sensorId));
          break;
        default:
          Serial.println("ERROR: Invalid command...");
          break;
      }
    }
  }
}


/* toStringTouchData(touchData_t, String*)*/
// Appends touch data to string passed in by reference
void toStringTouchData(touchData_t touchData, String * str)
{
  if(touchData.mode == ABSOLUTE)
  {
    str->concat(touchData.absolute.xValue);
    str->concat('\t');
    str->concat(touchData.absolute.yValue);
    str->concat('\t');
    str->concat(touchData.absolute.zValue);

    // If in curved overlay mode, print the hovering status.
    (touchData.overlayMode == 0) ? str->concat('\t') :
        (touchData.absolute.hovering) ? str->concat(" - h\t") :
        str->concat(" - v\t");
  }
  else
  {
    str->concat(touchData.relative.xDelta);
    str->concat('\t');
    str->concat(touchData.relative.yDelta);
    str->concat('\t');
    str->concat(touchData.relative.wheelCount);
    str->concat('\t');
  }
}

/* cyclePower() */
// This function cycles power for both Pinnacle devices
void cyclePower()
{
  Pinnacle_cyclePower(SENSOR_0);
  Pinnacle_cyclePower(SENSOR_1);
}

void printInstructions()
{
  Serial.println("Commands:");
  Serial.println("a - set to absolute mode");
  Serial.println("c - force sensor to recalibrate");
  Serial.println("d - disable the feed");
  Serial.println("e - enable the feed");
  Serial.println("f - enable curved overlay");
  Serial.println("g - disable curved overlay");
  Serial.println("m - get comp-matrix data");
  Serial.println("r - set to relative mode");
  Serial.println("s - toggle enable/disable sensor");
  Serial.println("l - list these commands again\n");
}

// Function for prompting and selecting the correct sensor.
uint8_t getSensorSelect()
{
  uint8_t val = 0xFF;
  uint8_t returnVal = 0xFF;

  Serial.println("Select sensor (0 or 1): ");

  // wait until a selection has been entered.
  while (Serial.available() <= 0);
  val = Serial.read();

  returnVal = (val == '0') ? 0x00 :
    (val == '1') ? 0x01 :
    0xFF;                             // Fail State

  return returnVal;
}

// Function to generate sensor enable status string.
String Sensor_toString(uint8_t sensorId)
{
  String dispStr = "";
  dispStr = "Sensor ";
  dispStr += String(sensorId);
  dispStr += " - ";
  dispStr += (senData[sensorId].senSel) ? "Enabled" : "Disabled";

  return dispStr;
}
