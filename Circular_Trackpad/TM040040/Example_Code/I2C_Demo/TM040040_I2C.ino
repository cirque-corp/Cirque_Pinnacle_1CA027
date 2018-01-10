#include <Wire.h>

// ___ Using a Cirque TM040040 with an Arduino ___
// This demonstration application is built to work with a Teensy 3.1/3.2 but it can easily be adapted to
// work with Arduino-based systems.
// This application connects to a TM040040 circular touch pad via I2C. To verify that your touch pad is configured 
// for I2C-mode, make sure that R1 is NOT populated (whichever resistor connects pins 24 & 25 of the 1CA027 IC).
// The pad is configured for Absolute mode tracking.  Touch data is sent in text format over USB CDC to
// the host PC.  You can open a terminal window on the PC to the USB CDC port and see X, Y, and Z data
// scroll up the window when you touch the sensor. Tools->Serial Monitor can be used to view touch data.

//  Pinnacle TM040040 with Arduino
//  Hardware Interface
//  GND
//  +3.3V
//  SDA = Pin 18  (there MUST be a pull-up to 3.3V on this signal; 4.7k recommended)
//  SCL = Pin 19  (there MUST be a pull-up to 3.3V on this signal; 4.7k recommended)
//  DR = Pin 9

// Hardware pin-number labels
#define SDA_PIN   18
#define SCL_PIN   19
#define DR_PIN    9

#define LED_0     21
#define LED_1     20

// Cirque's 7-bit I2C Slave Address
#define SLAVE_ADDR  0x2A

// Masks for Cirque Register Access Protocol (RAP)
#define WRITE_MASK  0x80
#define READ_MASK   0xA0

// Register config values for this demo
#define SYSCONFIG_1   0x00
#define FEEDCONFIG_1  0x03
#define FEEDCONFIG_2  0x1F
#define Z_IDLE_COUNT  0x05

// Coordinate scaling values
#define PINNACLE_XMAX     2047    // max value Pinnacle can report for X
#define PINNACLE_YMAX     1535    // max value Pinnacle can report for Y
#define PINNACLE_X_LOWER  127     // min "reachable" X value
#define PINNACLE_X_UPPER  1919    // max "reachable" X value
#define PINNACLE_Y_LOWER  63      // min "reachable" Y value
#define PINNACLE_Y_UPPER  1471    // max "reachable" Y value
#define PINNACLE_X_RANGE  (PINNACLE_X_UPPER-PINNACLE_X_LOWER)
#define PINNACLE_Y_RANGE  (PINNACLE_Y_UPPER-PINNACLE_Y_LOWER)

// Convenient way to store and access measurements
typedef struct _absData
{
  uint16_t xValue;
  uint16_t yValue;
  uint16_t zValue;
  uint8_t buttonFlags;
  bool touchDown;
} absData_t;

absData_t touchData;

// setup() gets called once at power-up, sets up serial debug output and Cirque's Pinnacle ASIC.
void setup()
{
  Serial.begin(115200);
  while(!Serial); // needed for USB
  Serial.println("X\tY\tZ\tButtons");

  pinMode(LED_0, OUTPUT);

  Pinnacle_Init();
}

// loop() continuously checks to see if data-ready (DR) is high. If so, reads and reports touch data to terminal.
void loop()
{
  if(DR_Asserted())
  {    
    Pinnacle_GetAbsolute(&touchData);
//    ScaleData(&touchData, 1024, 1024);  // Scale coordinates to arbitrary X, Y resolution
          
    Serial.print(touchData.xValue);
    Serial.print('\t');
    Serial.print(touchData.yValue);
    Serial.print('\t');
    Serial.print(touchData.zValue);
    Serial.print('\t');
    Serial.println(touchData.buttonFlags);
  }
  AssertSensorLED(touchData.touchDown);
}

/*  Pinnacle-based TM040040 Functions  */
void Pinnacle_Init()
{
  // Set up I2C peripheral
  Wire.begin();
  Wire.setClock(100000);

  pinMode(DR_PIN, INPUT);

  // Host clears SW_CC flag
  Pinnacle_ClearFlags();

  // Host configures bits of registers 0x03 and 0x05
  RAP_Write(0x03, SYSCONFIG_1);
  RAP_Write(0x05, FEEDCONFIG_2);

  // Host enables preferred output mode (absolute)
  RAP_Write(0x04, FEEDCONFIG_1);

  // Host sets z-idle packet count to 5 (default is 30)
  RAP_Write(0x0A, Z_IDLE_COUNT);
}

// Reads XYZ data from Pinnacle registers 0x14 through 0x17
// Stores result in absData_t struct with xValue, yValue, and zValue members
void Pinnacle_GetAbsolute(absData_t * result)
{
  uint8_t data[6] = { 0,0,0,0,0,0 };
  RAP_ReadBytes(0x12, data, 6);
  
  Pinnacle_ClearFlags();
  
  result->buttonFlags = data[0] & 0x3F;
  result->xValue = data[2] | ((data[4] & 0x0F) << 8);
  result->yValue = data[3] | ((data[4] & 0xF0) << 4);
  result->zValue = data[5] & 0x3F;

  result->touchDown = result->xValue != 0;
}

// Clears Status1 register flags (SW_CC and SW_DR)
void Pinnacle_ClearFlags()
{
  RAP_Write(0x02, 0x00);
  delayMicroseconds(50);
}

// Enables/Disables the feed
void Pinnacle_EnableFeed(bool feedEnable)
{
  uint8_t temp;

  RAP_ReadBytes(0x04, &temp, 1);  // Store contents of FeedConfig1 register
  
  if(feedEnable)
  {
    temp |= 0x01;                 // Set Feed Enable bit
    RAP_Write(0x04, temp);
  }
  else
  {
    temp &= ~0x01;                // Clear Feed Enable bit
    RAP_Write(0x04, temp);
  }
}

/*  ERA (Extended Register Access) Functions  */
// Reads <count> bytes from an extended register at <address> (16-bit address),
// stores values in <*data>
void ERA_ReadBytes(uint16_t address, uint8_t * data, uint16_t count)
{  
  uint8_t ERAControlValue = 0xFF;

  Pinnacle_EnableFeed(false); // Disable feed

  RAP_Write(0x1C, (uint8_t)(address >> 8));     // Send upper byte of ERA address
  RAP_Write(0x1D, (uint8_t)(address & 0x00FF)); // Send lower byte of ERA address
  
  for(uint16_t i = 0; i < count; i++)
  {
    RAP_Write(0x1E, 0x05);  // Signal ERA-read (auto-increment) to Pinnacle
    
    // Wait for status register 0x1E to clear
    do
    {
      RAP_ReadBytes(0x1E, &ERAControlValue, 1);
    } while(ERAControlValue != 0x00);
    
    RAP_ReadBytes(0x1B, data + i, 1);
    
    Pinnacle_ClearFlags();
  }
}

// Writes a byte, <data>, to an extended register at <address> (16-bit address)
void ERA_WriteByte(uint16_t address, uint8_t data)
{
  uint8_t ERAControlValue = 0xFF;

  Pinnacle_EnableFeed(false); // Disable feed

  RAP_Write(0x1B, data);      // Send data byte to be written
  
  RAP_Write(0x1C, (uint8_t)(address >> 8));     // Upper byte of ERA address
  RAP_Write(0x1D, (uint8_t)(address & 0x00FF)); // Lower byte of ERA address

  RAP_Write(0x1E, 0x02);  // Signal an ERA-write to Pinnacle

  // Wait for status register 0x1E to clear
  do
  {
    RAP_ReadBytes(0x1E, &ERAControlValue, 1);
  } while(ERAControlValue != 0x00);
    
  Pinnacle_ClearFlags();
}

/*  RAP Functions */
// Reads <count> Pinnacle registers starting at <address>
void RAP_ReadBytes(uint8_t address, uint8_t * data, uint8_t count)
{
  uint8_t cmdByte = READ_MASK | address;   // Form the READ command byte
  uint8_t i = 0;

  Wire.beginTransmission(SLAVE_ADDR);   // Set up an I2C-write to the I2C slave (Pinnacle)
  Wire.write(cmdByte);                  // Signal a RAP-read operation starting at <address>
  Wire.endTransmission(true);           // I2C stop condition

  Wire.requestFrom((uint8_t)SLAVE_ADDR, count, (uint8_t)true);  // Read <count> bytes from I2C slave
  while(Wire.available())
  {
    data[i++] = Wire.read();
  }
}

// Writes single-byte <data> to <address>
void RAP_Write(uint8_t address, uint8_t data)
{
  uint8_t cmdByte = WRITE_MASK | address;  // Form the WRITE command byte

  Wire.beginTransmission(SLAVE_ADDR);   // Set up an I2C-write to the I2C slave (Pinnacle)
  Wire.send(cmdByte);                   // Signal a RAP-write operation at <address>
  Wire.send(data);                      // Write <data> to I2C slave
  Wire.endTransmission(true);           // I2C stop condition
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
bool DR_Asserted()
{
  return digitalRead(DR_PIN);
}

void AssertSensorLED(bool state)
{
  digitalWrite(LED_0, !state);
}
