#include <SPI.h>

// ___ Using a Cirque TM040040 with an Arduino ___
// This demonstration application is built to work with a Teensy 3.1/3.2 but it can easily be adapted to
// work with Arduino-based systems.
// This application connects to a TM040040 circular touch pad via SPI. To verify that your touch pad is configured 
// for SPI-mode, make sure that R1 is populated with a 470k resistor (or whichever resistor connects pins 24 & 25 of the 1CA027 IC).
// The pad is configured for Absolute mode tracking.  Touch data is sent in text format over USB CDC to
// the host PC.  You can open a terminal window on the PC to the USB CDC port and see X, Y, and Z data
// scroll up the window when you touch the sensor. Tools->Serial Monitor can be used to view touch data.

//  Pinnacle TM040040 with Arduino
//  Hardware Interface
//  GND
//  +3.3V
//  SCK = Pin 13
//  MISO = Pin 12
//  MOSI = Pin 11
//  SS = PIN 10
//  DR = Pin 9

// Hardware pin-number labels
#define SCK_PIN   13
#define DIN_PIN   12
#define DOUT_PIN  11
#define CS_PIN    10
#define DR_PIN    9

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
  DeAssert_CS();
  pinMode(CS_PIN, OUTPUT);
  pinMode(DR_PIN, INPUT);

  SPI.begin();

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
  result->zValue = data[3] & 0x3F;
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

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE1));
  
  Assert_CS();
  SPI.transfer(cmdByte);  // Signal a RAP-read operation starting at <address>
  SPI.transfer(0xFC);     // Filler byte
  SPI.transfer(0xFC);     // Filler byte
  for(byte i = 0; i < count; i++)
  {
    data[i] =  SPI.transfer(0xFC);  // Each subsequent SPI transfer gets another register's contents
  }
  DeAssert_CS();

  SPI.endTransaction();
}

// Writes single-byte <data> to <address>
void RAP_Write(byte address, byte data)
{
  byte cmdByte = WRITE_MASK | address;  // Form the WRITE command byte

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE1));

  Assert_CS();
  SPI.transfer(cmdByte);  // Signal a write to register at <address>
  SPI.transfer(data);    // Send <value> to be written to register
  DeAssert_CS();

  SPI.endTransaction();
}

/*  I/O Functions */
void Assert_CS()
{
  digitalWrite(CS_PIN, LOW);
}

void DeAssert_CS()
{
  digitalWrite(CS_PIN, HIGH);
}

bool DR_Asserted()
{
  return digitalRead(DR_PIN);
}
