#include <SPI.h>
#include "Pinnacle.h"

// ___ Using a Cirque 1CA027 (Pinnacle IC) with an Arduino to make cap-sense measurements___
// This demonstration application is built to work with a Teensy 3.1/3.2 but it can easily be adapted to
// work with any Arduino-based systems.
// This application connects to a XXXXXX break out board via SPI. 
// Normally this dev kit is used for tracking position on the pad using a module like the TM040040 or TM035035.  However, this demo is to show how to do "anymeas" functionality with this IC.
// Anymeas is the functionality where the cap touch IC is not used for its built in tracking measurements but rather as a measurement
// engine for doing custom measurements.  These measurements can be configured to get both touch/button information or for a level of proximity sensing as well.  

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

//  Pinnacle XXXXXX with Arduino
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

//LED0 and LED1 are the on board LEDs
#define LED_0     21
#define LED_1     20

// Masks for Cirque Register Access Protocol (RAP)
#define WRITE_MASK  0x80
#define READ_MASK   0xA0

#define ENABLE_SERIAL_DEBUG

typedef struct
{
  unsigned long Toggle;
  unsigned long Positive;
} MeasVector;

MeasVector Measurements[] =
{
  //bit order for toggle and polarity
  //   bit 31                 bit27      bit23    bit19    bit15        bit11      bit7     bit3   bit0
  //   NotUsedNotUsedRef1Ref0 Y11Y10Y9Y8 Y7Y6Y5Y4 Y3Y2Y1Y0 X15X14X13X12 X11X10X9X8 X7X6X5X4 X3X2X1X0
  {Toggle : 0x00010000, Positive : 0x00010000}, //This toggles Y0 only and toggles it positively
  {Toggle : 0x00010000, Positive : 0x00000000}, //This toggles Y0 only and toggles it negatively
  {Toggle : 0x00000001, Positive : 0x00000000}, //This toggles X0 only and toggles it positively
  {Toggle : 0x00008000, Positive : 0x00000000}, //This toggles X16 only and toggles it positively
  {Toggle : 0x00FF00FF, Positive : 0x000000FF}  //This toggles Y0-Y7 negative and X0-X7 positive
};

const int NumberMeasurements = sizeof(Measurements) / sizeof(Measurements[0]);

int MeasurementResults[NumberMeasurements];
int Compensation[NumberMeasurements];

// setup() gets called once at power-up, sets up serial debug output and Cirque's Pinnacle ASIC.
void setup()
{
  #ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
  while(!Serial); // needed for USB
  Serial.println("Initial Test");
  #endif

  Pinnacle_Init();
  SimpleCompInit();
}

void loop()
{
  int Signal[NumberMeasurements];
  int i;
  // Make all of the measurements
  for (i=0;i<NumberMeasurements;i++)
  {
    MeasurementResults[i] = ADC_TakeMeasurement(Measurements[i].Toggle, Measurements[i].Positive);
    // compensate the measurement
    Signal[i] = MeasurementResults[i] - Compensation[i];
  }

#ifdef ENABLE_SERIAL_DEBUG
  for (i=0;i<NumberMeasurements;i++)
  {
    Serial.print("Meas ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(Signal[i]);
    Serial.print("\t");
  } 
    Serial.println();        
#endif  
}

/* Init a simple compensation scheme where at first boot the measured values are used as the baseline to subtract a finger from on subsequent values */
void SimpleCompInit()
{
  int x;
  unsigned short i;
  signed short Value;
  signed long AccumValue;
  for (x = 0; x < NumberMeasurements; x++)
  {
    i = 0;
    AccumValue = 0;
    while (i < 5)  //take 5 measurements and average them for a bit lower noise compensation value
    {
        Value = ADC_TakeMeasurement(Measurements[x].Toggle, Measurements[x].Positive);
        i++;
        AccumValue += Value;
      }
    Compensation[x] = AccumValue / 5;
  }
}

/*  Iniitialize Pinnacle and setup in anymeas mode  */
void Pinnacle_Init()
{
  DeAssert_CS();
  pinMode(CS_PIN, OUTPUT);
  pinMode(DR_PIN, INPUT);

  SPI.begin();

  // Host clears SW_CC flag
  Pinnacle_ClearFlags();
  //disable tracking
  //This stops XY tracking measurements and allows us to be able to do any unique measurement.
  RAP_Write(HOSTREG__SYSCONFIG1__TRACK_DISABLE,HOSTREG__SYSCONFIG1);
  //Delay 10ms after the initial track disable to allow all tracking operations to complete.  
  delayMicroseconds(10000);
  PinnacleADC_Init();  
  Pinnacle_ClearFlags();
}


/* Initialize Pinnacle ADC, in anymeas mode the 32 RAP registers are re-used as ADC registers */
void PinnacleADC_Init()
{
  RAP_Write(AnyMeas_AccumBits_ElecFreq,(ADCCNFG_ACCBITS_17_14_0|ADCCNFG_EF_0));  //gain and EF-low gain, fastest EF
  RAP_Write(AnyMeas_BitLength,ADCCTRL_SAMPLES_512);  //BitLength
  RAP_Write(AnyMeas_ADC_MuxControl,ADCMUXCTRL_SENSENGATE);  //SenseMux
  RAP_Write(AnyMeas_ADC_Config2,0);  //ADCConfig2
  RAP_Write(AnyMeas_ADC_AWidth,ADCAWIDTH_APERTURE_500NS);  //ApertureWidth
  RAP_Write(HostReg__19,0);  //clear out toggle and polarity registers to start
  RAP_Write(HostReg__20,0);
  RAP_Write(HostReg__21,0);
  RAP_Write(HostReg__22,0);
  RAP_Write(HostReg__23,0);
  RAP_Write(HostReg__24,0);
  RAP_Write(HostReg__25,0);
  RAP_Write(HostReg__26,0);
  
  // these just point to regs below starting at 19
  RAP_Write(AnyMeas_pADCMeasInfoStart_High_Byte,0);
  RAP_Write(AnyMeas_pADCMeasInfoStart_Low_Byte,HostReg__19);  //put measurement data starting at register 19
  RAP_Write(AnyMeas_MeasIndex,0);
  RAP_Write(AnyMeas_State,0);
  
  //1 non repeating measurement
  RAP_Write(AnyMeas_Control_NumMeas,0x01);    
}

/* Set ADC config settings like gain, bit length, sense mux, etc */
void ADC_SetConfig( unsigned char AccumBits_ElecFreq, unsigned char BitLength, unsigned char SenseMux, unsigned char ADCConfig2,unsigned char ApertureWidth )
{
  RAP_Write(HostReg__5,AccumBits_ElecFreq);
  RAP_Write(HostReg__6,BitLength);
  RAP_Write(HostReg__7,SenseMux);
  RAP_Write(HostReg__8,ADCConfig2);
  RAP_Write(HostReg__9,ApertureWidth);
}

/* Take an actual measurement */
signed short ADC_TakeMeasurement( unsigned long Toggle, unsigned long Polarity )
{
  unsigned char temp8;
  unsigned short temp16;
  signed short K2_Track_ADC_Result;

  RAP_Write(HostReg__19,((unsigned char)((Toggle >> 24))));
  RAP_Write(HostReg__20,((unsigned char)((Toggle >> 16))));
  RAP_Write(HostReg__21,((unsigned char)((Toggle >> 8))));
  RAP_Write(HostReg__22,((unsigned char)Toggle));

  RAP_Write(HostReg__23,((unsigned char)((Polarity >> 24))));
  RAP_Write(HostReg__24,((unsigned char)((Polarity >> 16))));
  RAP_Write(HostReg__25,((unsigned char)((Polarity >> 8))));
  RAP_Write(HostReg__26,((unsigned char)Polarity));
  
  //Start the measurement
  RAP_Write(HostReg__3,0x18);

  //!!!!! BLOCKS ON RESULT HERE
  //now wait for DR to go high and then get the measurement result
  while(!DR_Asserted());      //consider adding a counter, or enabling a watchdog to avoid being stuck forever

  RAP_ReadBytes(HostReg__17,&temp8,1);
  temp16 = (unsigned short)temp8;
  temp16 <<= 8;
  RAP_ReadBytes(HostReg__18,&temp8,1);
  temp16 |= temp8;
  
  //clear DR
  Pinnacle_ClearFlags();

  K2_Track_ADC_Result = (signed short)temp16;
  return (K2_Track_ADC_Result);
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

void AssertSensorLED0(bool state)
{
  digitalWrite(LED_0, !state);
}

void AssertSensorLED1(bool state)
{
  digitalWrite(LED_1, !state);
}

bool DR_Asserted()
{
  return digitalRead(DR_PIN);
}
