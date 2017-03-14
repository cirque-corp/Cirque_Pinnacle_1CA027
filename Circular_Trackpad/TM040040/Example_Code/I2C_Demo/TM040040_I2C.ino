#include <Wire.h>

// ___ Using a Cirque TM040040 with an Arduino ___
// This demonstration application is built to work with a Teensy 3.1/3.2 but it can easily be adapted to
// work with Arduino-based systems.
// This application connects to a TM040040 circular touch pad.
// The pad is configured for Absolute mode tracking.  Touch data is sent in text format over USB CDC to
// the host PC.  You can open a terminal window on the PC to the USB CDC port and see X, Y, and Z data
// scroll up the window when you touch the sensor. Tools->Serial Monitor can be used to view touch data.

//  Pinnacle TM040040 with Arduino
//  Hardware Interface
//  GND
//  +3.3V
//  SDA = Pin 18
//  SCL = Pin 19
//  DR = Pin 9

// Hardware pin-number labels
#define SDA_PIN   18
#define SCL_PIN   19
#define DR_PIN    9

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

// Convenient way to store and access measurements
typedef struct _absData
{
  unsigned int xValue;
  unsigned int yValue;
  unsigned int zValue;
} absData_t;

absData_t touchData;


// setup() gets called once at power-up, sets up serial debug output and Cirque's Pinnacle ASIC.
void setup()
{
  Serial.begin(115200);
  while(!Serial); // needed for USB
  Serial.println("X\tY\tZ");

  Pinnacle_Init();
}

// loop() continuously checks to see if data-ready (DR) is high. If so, reads and reports touch data to terminal.
void loop()
{ 
  if(DR_Asserted())
  {    
    Pinnacle_GetTouchPackets(&touchData);
    
    Serial.print(touchData.xValue);
    Serial.print('\t');
    Serial.print(touchData.yValue);
    Serial.print('\t');
    Serial.println(touchData.zValue);
  }
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

// Merges X_LOW_BYTE, Y_LOW_BYTE, and XY_HIGH_BITS into X,Y,Z coordinates. 
// Operates on contents of Pinnacle registers 0x14 through 0x17
void Pinnacle_ParseAbsolute(byte * data, absData_t * result)
{
  result->xValue = data[0] | ((data[2] & 0x0F) << 8);
  result->yValue = data[1] | ((data[2] & 0xF0) << 4);
  result->zValue = data[3];
}

// Reads XYZ data from Pinnacle registers 0x14 through 0x17
// Stores result in absData_t struct with xValue, yValue, and zValue members
void Pinnacle_GetTouchPackets(absData_t * result)
{
  byte data[4] = { 0,0,0,0 };
  RAP_ReadBytes(0x14, data, 4);
  Pinnacle_ClearFlags();
  Pinnacle_ParseAbsolute(data, result);
}

// Clears Status1 register flags (SW_CC and SW_DR)
void Pinnacle_ClearFlags()
{
  RAP_Write(0x02, 0x00);
}

/*  RAP Functions */
// Reads <count> Pinnacle registers starting at <address>
void RAP_ReadBytes(byte address, byte * data, byte count)
{
  byte cmdByte = READ_MASK | address;   // Form the READ command byte
  byte i = 0;

  Wire.beginTransmission(SLAVE_ADDR);   // Set up an I2C-write to the I2C slave (Pinnacle)
  Wire.write(cmdByte);                  // Signal a RAP-read operation starting at <address>
  Wire.endTransmission(true);           // I2C stop condition

  Wire.requestFrom((byte)SLAVE_ADDR, count, (byte)true);  // Read <count> bytes from I2C slave
  while(Wire.available())
  {
    data[i++] = Wire.read();
  }
}

// Writes single-byte <data> to <address>
void RAP_Write(byte address, byte data)
{
  byte cmdByte = WRITE_MASK | address;  // Form the WRITE command byte

  Wire.beginTransmission(SLAVE_ADDR);   // Set up an I2C-write to the I2C slave (Pinnacle)
  Wire.send(cmdByte);                   // Signal a RAP-write operation at <address>
  Wire.send(data);                      // Write <data> to I2C slave
  Wire.endTransmission(true);           // I2C stop condition
}

/*  I/O Functions */
bool DR_Asserted()
{
  return digitalRead(DR_PIN);
}
