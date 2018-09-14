// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <SPI.h>
#include <string.h>

// ___ Using a Cirque TM0XX0XX and TM0XX0XX with a DK-000013-0x and Arduino ___
// This demonstration application is built to work with a Teensy 3.1/3.2 but it can easily be adapted to
// work with Arduino-based systems.
// This application connects to a TM0XX0XX (Sensor0) and TM0XX0XX (Sensor1) circular touch pad via SPI. 
// To verify that your touch pad is configured for SPI-mode, make sure that R1 is populated with a 470k resistor
// (or whichever resistor connects pins 24 & 25 of the 1CA027 IC).
// The pad is configured for Absolute mode tracking.  Touch data is sent in text format over USB CDC to
// the host PC.  You can open a terminal window on the PC to the USB CDC port and see X, Y, and Z data
// fill the window when you touch the sensor.
// In the Arduino IDE use Tools->Serial Monitor to view touch data.
// This demo can use both sensor ports on the 02-000620-00REVA00 development board.
// You can configure which sensors are active using the SENSE0_SELECT and SENSE0_SELECT shown below.
// You can configure curved overlay or flat overlay using SENSE0_OVERLAY_CURVE and SENSE1_OVERLAY_CURVE shown below.

// NOTE: all config values applied in this sample are meant for a module using REXT = 976kOhm

//  Pinnacle TM0XX0XX with Arduino
//  Hardware Interface
//  GND
//  +3.3V
//  SCK = Pin 13
//  MISO = Pin 12
//  MOSI = Pin 11
//  SS = Pin 10 - or - Pin 8
//  DR = Pin 9  - or - Pin 7

// Hardware pin-number labels
// SPI port definitions
#define SCK_PIN   13
#define DIN_PIN   12
#define DOUT_PIN  11
#define CS0_PIN   10    // Chip Select for Sensor 0
#define CS1_PIN   8     // Chip Select for Sensor 1

// Pinnacle Data-Ready pins
#define DR0_PIN   9     // Data-Ready for Sensor 0
#define DR1_PIN   7     // Data-Ready for Sensor 1

// I2C pins (not used in this demo)
#define SDA_PIN   18
#define SCL_PIN   19

#define LED_0     21
#define LED_1     20

// Masks for Cirque Register Access Protocol (RAP)
#define WRITE_MASK  0x80
#define READ_MASK   0xA0

// Register config values for this demo
#define SYSCONFIG_1   0x00
#define FEEDCONFIG_1  0x03
#define FEEDCONFIG_2  0x1F
#define Z_IDLE_COUNT  0x05

//const uint16_t ZONESCALE = 256;  // 256 position steps between electrodes
//const uint16_t ROWS_Y = 6;  // Number of Y electrodes
//const uint16_t COLS_X = 8;  // Number of X electrodes

// Coordinate scaling values
#define PINNACLE_XMAX     2047    // max value Pinnacle can report for X (0 to (8 * 256) - 1)
#define PINNACLE_YMAX     1535    // max value Pinnacle can report for Y (0 to (6 * 256) - 1)
#define PINNACLE_X_LOWER  127     // min "reachable" X value
#define PINNACLE_X_UPPER  1919    // max "reachable" X value
#define PINNACLE_Y_LOWER  63      // min "reachable" Y value
#define PINNACLE_Y_UPPER  1471    // max "reachable" Y value
#define PINNACLE_X_RANGE  (PINNACLE_X_UPPER-PINNACLE_X_LOWER)
#define PINNACLE_Y_RANGE  (PINNACLE_Y_UPPER-PINNACLE_Y_LOWER)
#define ZONESCALE 256   // divisor for reducing x,y values to an array index for the LUT
#define ROWS_Y ((PINNACLE_YMAX + 1) / ZONESCALE)
#define COLS_X ((PINNACLE_XMAX + 1) / ZONESCALE)

// ADC-attenuation settings (held in BIT_7 and BIT_6)
// 1X = most sensitive, 4X = least sensitive
#define ADC_ATTENUATE_1X   0x00
#define ADC_ATTENUATE_2X   0x40
#define ADC_ATTENUATE_3X   0x80
#define ADC_ATTENUATE_4X   0xC0

// Select sensors that are active
// 1 = On, 0 = Off
#define SENSE0_SELECT         1
#define SENSE1_SELECT         1
// Select the overlay type
// 1 = Curved Overlay, 0 = Flat Overlay
#define SENSE0_OVERLAY_CURVE  1
#define SENSE1_OVERLAY_CURVE  1

// Convenient way to store and access measurements
typedef struct _absData
{
  uint16_t xValue;
  uint16_t yValue;
  uint16_t zValue;
  bool touchDown;
  bool hovering;
} absData_t;

absData_t touchData_Sense0;
absData_t touchData_Sense1;

// Used to differentiate between the two sensors
typedef struct _padData
{
  uint8_t CS_Pin;
  uint8_t DR_Pin;
  uint8_t LED_Pin;
} padData_t;

padData_t Pad_Sense0;
padData_t Pad_Sense1;

// These values require tuning for optimal touch-response
// Each element represents the Z-value below which is considered "hovering" in that XY region of the sensor.
// The values present are not guaranteed to work for all HW configurations.
const uint8_t ZVALUE_MAP[ROWS_Y][COLS_X] =
{
  {0, 0,  0,  0,  0,  0, 0, 0},
  {0, 2,  3,  5,  5,  3, 2, 0},
  {0, 3,  5, 15, 15,  5, 2, 0},
  {0, 3,  5, 15, 15,  5, 3, 0},
  {0, 2,  3,  5,  5,  3, 2, 0},
  {0, 0,  0,  0,  0,  0, 0, 0},
};

// setup() gets called once at power-up, sets up serial debug output and Cirque's Pinnacle ASIC.
void setup()
{
  Serial.begin(115200);
  while(!Serial); // needed for USB
  String str = "";

  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);

  Pad_Sense0.CS_Pin = CS0_PIN;
  Pad_Sense0.DR_Pin = DR0_PIN;
  Pad_Sense0.LED_Pin = LED_0;

  Pad_Sense1.CS_Pin = CS1_PIN;
  Pad_Sense1.DR_Pin = DR1_PIN;
  Pad_Sense1.LED_Pin = LED_1;

  if(SENSE0_SELECT) Pinnacle_Init(&Pad_Sense0);
  if(SENSE1_SELECT) Pinnacle_Init(&Pad_Sense1);

  // These functions are required for use with thick overlays (curved)
  if(SENSE0_OVERLAY_CURVE)
  {
    setAdcAttenuation(ADC_ATTENUATE_2X, &Pad_Sense0);
    tuneEdgeSensitivity(&Pad_Sense0);
  }

  if(SENSE1_OVERLAY_CURVE)
  {
    setAdcAttenuation(ADC_ATTENUATE_2X, &Pad_Sense1);
    tuneEdgeSensitivity(&Pad_Sense0);
  }

  Serial.println();
  str = (SENSE1_SELECT && SENSE0_SELECT) ? ("\tX\tY\tZ\t\tX\tY\tZ") :
    (SENSE1_SELECT) ? ("SENSE 1\tX\tY\tZ") :
    (SENSE0_SELECT) ? ("SENSE 0\tX\tY\tZ") :
    ("BOTH SENSORS DISABLED .. ENABLE SENSOR SELECT");
  Serial.println(str);

  Pinnacle_EnableFeed(true, &Pad_Sense0);
  Pinnacle_EnableFeed(true, &Pad_Sense1);
}

// loop() continuously checks to see if data-ready (DR) is high. If so, reads and reports touch data to terminal.
void loop()
{
  String printData = "";

  // Note: the two Pinnacles are not synchronized. In a polling loop like this you
  // may get one or both of the sensors reporting new data. We just grab what data
  // there is and write it.
  if(DR_Asserted(&Pad_Sense0) && SENSE0_SELECT)
  {

    Pinnacle_GetAbsolute(&touchData_Sense0, &Pad_Sense0);

    Pinnacle_CheckValidTouch(&touchData_Sense0);     // Checks for "hover" caused by curved overlays

    ScaleData(&touchData_Sense0, 1024, 1024);      // Scale coordinates to arbitrary X, Y resolution

    printData += "SENS_0 ";

    Pinnacle_DataToString(&touchData_Sense0, &printData);
  }

  if(DR_Asserted(&Pad_Sense1) && SENSE1_SELECT)
  {
    Pinnacle_GetAbsolute(&touchData_Sense1, &Pad_Sense1);

    Pinnacle_CheckValidTouch(&touchData_Sense1);     // Checks for "hover" caused by curved overlays

    ScaleData(&touchData_Sense1, 1024, 1024);      // Scale coordinates to arbitrary X, Y resolution

    printData += (SENSE0_SELECT && printData.length() == 0) ? ("\t\t\t\tSENS_1 ") :
      (SENSE0_SELECT) ? "\tSENS_1 " :
       "SENS_1 ";

    Pinnacle_DataToString(&touchData_Sense1, &printData);
  }

  if (printData.length() != 0)
  {
      // if there is data to write then write it
      printData += "\n";
      Serial.print(printData);
  }

  AssertSensorLED(touchData_Sense0.touchDown, Pad_Sense0.LED_Pin);
  AssertSensorLED(touchData_Sense1.touchDown, Pad_Sense1.LED_Pin);
}

// General Print function to display the parameters
void Pinnacle_DataToString(absData_t * touchData, String * str)
{
  str->concat(String(touchData->xValue));
  str->concat("\t");
  str->concat(String(touchData->yValue));
  str->concat("\t");
  str->concat(String(touchData->zValue));
  str->concat("-");

  if(Pinnacle_zIdlePacket(touchData))
  {
    str->concat("L ");       // append 'Liftoff' code to end of string
  }
  else if(touchData->hovering)
  {
    str->concat("H ");      // append 'Hover' code to end of string
  }
  else
  {
    str->concat("V ");      // append 'Valid' code to end of string
  }
}

/*  Pinnacle functions  */
void Pinnacle_Init(padData_t * currPad)
{
  RAP_Init(currPad);
  DeAssert_CS(currPad->CS_Pin);
  pinMode(currPad->DR_Pin, INPUT);

  // Host clears SW_CC flag
  Pinnacle_ClearFlags(currPad);

  // Host configures bits of registers 0x03 and 0x05
  RAP_Write(0x03, SYSCONFIG_1, currPad->CS_Pin);
  RAP_Write(0x05, FEEDCONFIG_2, currPad->CS_Pin);

  // Host enables preferred output mode (absolute)
  RAP_Write(0x04, FEEDCONFIG_1, currPad->CS_Pin);

  // Host sets z-idle packet count to 5 (default is 30)
  RAP_Write(0x0A, Z_IDLE_COUNT, currPad->CS_Pin);
  Serial.println("Pinnacle Initialized...");
}

// Reads XYZ data from Pinnacle registers 0x14 through 0x17
// Stores result in absData_t struct with xValue, yValue, and zValue members
void Pinnacle_GetAbsolute(absData_t * result, padData_t * currPad)
{
  uint8_t data[4] = { 0,0,0,0 };
  RAP_ReadBytes(0x14, data, 4, currPad->CS_Pin);

  Pinnacle_ClearFlags(currPad);

  result->xValue = data[0] | ((data[2] & 0x0F) << 8);
  result->yValue = data[1] | ((data[2] & 0xF0) << 4);
  result->zValue = data[3] & 0x3F;  // mask off upper two bits (reserved functionality)

  result->touchDown = result->xValue != 0;
}

// Checks touch data to see if it is a z-idle packet (all zeros)
bool Pinnacle_zIdlePacket(absData_t * data)
{
  return data->xValue == 0 && data->yValue == 0 && data->zValue == 0;
}

// Clears Status1 register flags (SW_CC and SW_DR)
void Pinnacle_ClearFlags(padData_t * currPad)
{
  RAP_Write(0x02, 0x00, currPad->CS_Pin);
  delayMicroseconds(50);
}

// Enables/Disables the feed
void Pinnacle_EnableFeed(bool feedEnable, padData_t * currPad)
{
  uint8_t temp;

  RAP_ReadBytes(0x04, &temp, 1, currPad->CS_Pin);  // Store contents of FeedConfig1 register

  if(feedEnable)
  {
    temp |= 0x01;                 // Set Feed Enable bit
    RAP_Write(0x04, temp, currPad->CS_Pin);
  }
  else
  {
    temp &= ~0x01;                // Clear Feed Enable bit
    RAP_Write(0x04, temp, currPad->CS_Pin);
  }
}

/*  Curved Overlay Functions  */
// Adjusts the feedback in the ADC, effectively attenuating the finger signal
// By default, the the signal is maximally attenuated (ADC_ATTENUATE_4X for use with thin, flat overlays
void setAdcAttenuation(uint8_t adcGain, padData_t * currPad)
{
  uint8_t temp = 0x00;

  Serial.println();
  Serial.println("Setting ADC gain...");
  ERA_ReadBytes(0x0187, &temp, 1, currPad);
  temp &= 0x3F; // clear top two bits
  temp |= adcGain;
  ERA_WriteByte(0x0187, temp, currPad);
  ERA_ReadBytes(0x0187, &temp, 1, currPad);
  Serial.print("ADC gain set to:\t");
  Serial.print(temp &= 0xC0, HEX);
  switch(temp)
  {
    case ADC_ATTENUATE_1X:
      Serial.println(" (X/1)");
      break;
    case ADC_ATTENUATE_2X:
      Serial.println(" (X/2)");
      break;
    case ADC_ATTENUATE_3X:
      Serial.println(" (X/3)");
      break;
    case ADC_ATTENUATE_4X:
      Serial.println(" (X/4)");
      break;
    default:
      break;
  }
}

// Changes thresholds to improve detection of fingers
void tuneEdgeSensitivity(padData_t * currPad)
{
  uint8_t temp = 0x00;

  Serial.println();
  Serial.println("Setting xAxis.WideZMin...");
  ERA_ReadBytes(0x0149, &temp, 1, currPad);
  Serial.print("Current value:\t");
  Serial.println(temp, HEX);
  ERA_WriteByte(0x0149,  0x04, currPad);
  ERA_ReadBytes(0x0149, &temp, 1, currPad);
  Serial.print("New value:\t");
  Serial.println(temp, HEX);

  Serial.println();
  Serial.println("Setting yAxis.WideZMin...");
  ERA_ReadBytes(0x0168, &temp, 1, currPad);
  Serial.print("Current value:\t");
  Serial.println(temp, HEX);
  ERA_WriteByte(0x0168,  0x03, currPad);
  ERA_ReadBytes(0x0168, &temp, 1, currPad);
  Serial.print("New value:\t");
  Serial.println(temp, HEX);
}

// This function identifies when a finger is "hovering" so your system can choose to ignore it.
// The sensor detects the finger in the space above the sensor. If the finger is on the surface of the sensor the Z value is highest.
// If the finger is a few millimeters above the surface the z value is much lower.
// Adding a curved overlay will allow the finger to be closer in the middle (so a higher z value) but farther
// on the perimeter (so a lower z value).
// With a curved overlay you tune the gain of the system to see a finger on the perimeter of the sensor
// (the finger is farther away).  Unfortunately a finger near the center will be detected above the surface.
// This code will tell you when to ignore that "hovering" finger.
// ZVALUE_MAP[][] stores a lookup table in which you can define the Z-value and XY position that is considered "hovering". Experimentation/tuning is required.
// NOTE: Z-value output decreases to 0 as you move your finger away from the sensor, and it's maximum value is 0x63 (6-bits).
void Pinnacle_CheckValidTouch(absData_t * touchData)
{
  uint32_t zone_x, zone_y;
  //eliminate hovering
  zone_x = touchData->xValue / ZONESCALE;
  zone_y = touchData->yValue / ZONESCALE;
  touchData->hovering = !(touchData->zValue > ZVALUE_MAP[zone_y][zone_x]);
}

/*  ERA (Extended Register Access) Functions  */
// Reads <count> bytes from an extended register at <address> (16-bit address),
// stores values in <*data>
void ERA_ReadBytes(uint16_t address, uint8_t * data, uint16_t count, padData_t * currPad)
{
  uint8_t ERAControlValue = 0xFF;

  Pinnacle_EnableFeed(false, currPad); // Disable feed

  RAP_Write(0x1C, (uint8_t)(address >> 8), currPad->CS_Pin);     // Send upper byte of ERA address
  RAP_Write(0x1D, (uint8_t)(address & 0x00FF), currPad->CS_Pin); // Send lower byte of ERA address

  for(uint16_t i = 0; i < count; i++)
  {
    RAP_Write(0x1E, 0x05, currPad->CS_Pin);  // Signal ERA-read (auto-increment) to Pinnacle

    // Wait for status register 0x1E to clear
    do
    {
      RAP_ReadBytes(0x1E, &ERAControlValue, 1, currPad->CS_Pin);
    } while(ERAControlValue != 0x00);

    RAP_ReadBytes(0x1B, data + i, 1, currPad->CS_Pin);

    Pinnacle_ClearFlags(currPad);
  }
}

// Writes a byte, <data>, to an extended register at <address> (16-bit address)
void ERA_WriteByte(uint16_t address, uint8_t data, padData_t * currPad)
{
  uint8_t ERAControlValue = 0xFF;

  Pinnacle_EnableFeed(false, currPad); // Disable feed

  RAP_Write(0x1B, data, currPad->CS_Pin);      // Send data byte to be written

  RAP_Write(0x1C, (uint8_t)(address >> 8), currPad->CS_Pin);     // Upper byte of ERA address
  RAP_Write(0x1D, (uint8_t)(address & 0x00FF), currPad->CS_Pin); // Lower byte of ERA address

  RAP_Write(0x1E, 0x02, currPad->CS_Pin);  // Signal an ERA-write to Pinnacle

  // Wait for status register 0x1E to clear
  do
  {
    RAP_ReadBytes(0x1E, &ERAControlValue, 1, currPad->CS_Pin);
  } while(ERAControlValue != 0x00);

  Pinnacle_ClearFlags(currPad);
}

/*  RAP Functions */

void RAP_Init(padData_t *currPad)
{
  pinMode(currPad->CS_Pin, OUTPUT);
  SPI.begin();
}

// Reads <count> Pinnacle registers starting at <address>
void RAP_ReadBytes(byte address, byte * data, byte count, uint8_t currCSPin)
{
  byte cmdByte = READ_MASK | address;   // Form the READ command byte

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE1));

  Assert_CS(currCSPin);
  SPI.transfer(cmdByte);  // Signal a RAP-read operation starting at <address>
  SPI.transfer(0xFC);     // Filler byte
  SPI.transfer(0xFC);     // Filler byte
  for(byte i = 0; i < count; i++)
  {
    data[i] =  SPI.transfer(0xFC);  // Each subsequent SPI transfer gets another register's contents
  }
  DeAssert_CS(currCSPin);

  SPI.endTransaction();
}

// Writes single-byte <data> to <address>
void RAP_Write(byte address, byte data, uint8_t currCSPin)
{
  byte cmdByte = WRITE_MASK | address;  // Form the WRITE command byte

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE1));

  Assert_CS(currCSPin);
  SPI.transfer(cmdByte);  // Signal a write to register at <address>
  SPI.transfer(data);    // Send <value> to be written to register
  DeAssert_CS(currCSPin);

  SPI.endTransaction();
}

/*  Logical Scaling Functions */
// Clips raw coordinates to "reachable" window of sensor
// NOTE: values outside this window can only appear as a result of noise
void ClipCoordinates(absData_t * coordinates)
{
  if(coordinates->xValue < PINNACLE_X_LOWER)
  {
    coordinates->xValue = PINNACLE_X_LOWER;
  }
  else if(coordinates->xValue > PINNACLE_X_UPPER)
  {
    coordinates->xValue = PINNACLE_X_UPPER;
  }
  if(coordinates->yValue < PINNACLE_Y_LOWER)
  {
    coordinates->yValue = PINNACLE_Y_LOWER;
  }
  else if(coordinates->yValue > PINNACLE_Y_UPPER)
  {
    coordinates->yValue = PINNACLE_Y_UPPER;
  }
}

// Scales data to desired X & Y resolution
void ScaleData(absData_t * coordinates, uint16_t xResolution, uint16_t yResolution)
{
  uint32_t xTemp = 0;
  uint32_t yTemp = 0;

  ClipCoordinates(coordinates);

  xTemp = coordinates->xValue;
  yTemp = coordinates->yValue;

  // translate coordinates to (0, 0) reference by subtracting edge-offset
  xTemp -= PINNACLE_X_LOWER;
  yTemp -= PINNACLE_Y_LOWER;

  // scale coordinates to (xResolution, yResolution) range
  coordinates->xValue = (uint16_t)(xTemp * xResolution / PINNACLE_X_RANGE);
  coordinates->yValue = (uint16_t)(yTemp * yResolution / PINNACLE_Y_RANGE);
}

/*  I/O Functions */
void Assert_CS(uint8_t currPin)
{
  digitalWrite(currPin, LOW);
}

void DeAssert_CS(uint8_t currPin)
{
  digitalWrite(currPin, HIGH);
}

void AssertSensorLED(bool state, uint8_t CURR_LED)
{
  digitalWrite(CURR_LED, !state);
}

bool DR_Asserted(padData_t * padData)
{
  return digitalRead(padData->DR_Pin);
}
