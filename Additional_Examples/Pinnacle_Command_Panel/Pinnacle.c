// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdint.h>
#include <stdbool.h>
#include "Pinnacle.h"
#include "Hardware.h"

// Masks for Cirque Register Access Protocol (RAP)
#define WRITE_MASK  0x80
#define READ_MASK   0xA0

// Values used to toggle the startup sequence of Pinnacle
#define POWER_OFF_CMD  0x02
#define POWER_ON_CMD   0x00

#define COMP_MATRIX_ADDRESS 0x01DF
#define COMP_MATRIX_SIZE    92      // 92 bytes (46 int16_t values)



uint8_t _mode = ABSOLUTE;
uint8_t _overlayMode = FLAT;

// These values require tuning for optimal touch-response
// Each element represents the Z-value below which is considered "hovering" in that XY region of the sensor.
// The values present are not guaranteed to work for all HW configurations.
const uint8_t Z_TRESH[ROWS_Y][COLS_X] =
{
  {0, 0,  0,  0,  0,  0, 0, 0},
  {0, 2,  3,  5,  5,  3, 2, 0},
  {0, 3,  5, 15, 15,  5, 2, 0},
  {0, 3,  5, 15, 15,  5, 3, 0},
  {0, 2,  3,  5,  5,  3, 2, 0},
  {0, 0,  0,  0,  0,  0, 0, 0},
};

void Pinnacle_getAbsolute(touchData_t *, uint8_t);
void Pinnacle_getRelative(touchData_t *, uint8_t);

void Pinnacle_init(touchData_t * touchData, uint8_t sensorId)
{
  HW_deAssertCS(sensorId);
  Pinnacle_clearFlags(sensorId);

  // Host disables taps, secondary tap, scroll, and GlideExtend(R)
  // NOTE: these features pertain to relative mode only
  //  RAP_write(FEED_CONFIG_2, 0x1E);

  // Setting System configuration bits.
  Pinnacle_setZIdleCount(5, sensorId);
  Pinnacle_enableFeed(true, sensorId);
  Pinnacle_setToAbsolute(touchData, sensorId);
}

// Returns true if the data-ready signal (DR) is asserted
bool Pinnacle_available(uint8_t sensorId)
{
  return HW_drAsserted(sensorId);
}

/* Pinnacle_cyclePower() */
// This function turns off the given sensor.
void Pinnacle_cyclePower(uint8_t sensorId)
{
  RAP_write(SYS_CONFIG_1, POWER_OFF_CMD, sensorId);
  TIMER_delayMicroseconds(500);
  RAP_write(SYS_CONFIG_1, POWER_ON_CMD, sensorId);
}


/* Pinnacle_sensorPresent(uint8_t) */
// Attempts to write and then read from the Z-Idle Reg to see if <sensorId> is
// present. Returns true on success and false on fail.
bool Pinnacle_sensorPresent(uint8_t sensorId)
{
  uint8_t temp = 0x00;
  RAP_write(Z_IDLE, 0x00, sensorId);
  TIMER_delayMicroseconds(500);
  RAP_readBytes(Z_IDLE, &temp, 1, sensorId);

  return (temp == 0x00) ? true : false;
}

// This function handles DR (data-ready signal) events by reading out the touch packets from Pinnacle.
// Pinnacle.c keeps track of which mode the Pinnacle ASIC is in (ABSOLUTE, or RELATIVE) and
// reads the corresponding packet into <touchData>. <touchData.mode> signals to the caller which
// data was updated; <touchData.absolute> or <touchData.relative>.
// Call this function if(Pinnacle_available() == true)
void Pinnacle_getTouchData(touchData_t * touchData, uint8_t sensorId)
{
  if(touchData->mode == ABSOLUTE)
  {
    Pinnacle_getAbsolute(touchData, sensorId);
  }
  else
  {
    Pinnacle_getRelative(touchData, sensorId);
  }
}

// Clears Status1 register flags (SW_CC and SW_DR)
void Pinnacle_clearFlags(uint8_t sensorId)
{
  RAP_write(STATUS_1, 0x00, sensorId);
  TIMER_delayMicroseconds(50);
}

// Changes output to absolute (x, y, z) mode
void Pinnacle_setToAbsolute(touchData_t * touchData, uint8_t sensorId)
{
  uint8_t temp;

  RAP_readBytes(FEED_CONFIG_1, &temp, 1, sensorId);
  temp |= 0x02;
  RAP_write(FEED_CONFIG_1, temp, sensorId);

  touchData->mode = ABSOLUTE;
}

// Changes output to relative (deltaX, deltaY, deltaScroll) mode
void Pinnacle_setToRelative(touchData_t * touchData, uint8_t sensorId)
{
    uint8_t temp;

    if (touchData->overlayMode) Pinnacle_enableCurved(touchData, false, sensorId);   // if in curved mode, return to flat mode

    RAP_readBytes(FEED_CONFIG_1, &temp, 1, sensorId);
    temp &= ~0x02;
    RAP_write(FEED_CONFIG_1, temp, sensorId);

    touchData->mode = RELATIVE;
}

// Enables curved mode by changing the ADC attenuation and setting the <overlayMode>
void Pinnacle_enableCurved(touchData_t * touchData, bool curveEnable, uint8_t sensorId)
{
    if (curveEnable)
    {
        // Only enable curved overlay mode when the device is in ABSOLUTE mode
        // If not in ABSOLUTE mode the, the threshold matrix will not be able to
        //    be applied to the results and the touch pad will register false
        //    hovering events.
        if(touchData->mode == ABSOLUTE)
        {
            Pinnacle_setAdcAttenuation(ADC_ATTENUATE_2X, sensorId);
            touchData->overlayMode = CURVED;
        }
    }
    else
    {
        Pinnacle_setAdcAttenuation(ADC_ATTENUATE_4X, sensorId);
        touchData->overlayMode = FLAT;
    }
}

// Sets the number of Z-idle packets to be sent when liftoff is detected
// NOTE: Z-idle packets contains all zero values and are useful for detecting rapid taps
void Pinnacle_setZIdleCount(uint8_t count, uint8_t sensorId)
{
  RAP_write(Z_IDLE, count, sensorId);
}

// Enables or disables feed of touch data
void Pinnacle_enableFeed(bool feedEnable, uint8_t sensorId)
{
  uint8_t temp;

  RAP_readBytes(FEED_CONFIG_1, &temp, 1, sensorId);  // Store contents of FeedConfig1 register

  if(feedEnable)
  {
    temp |= 0x01;                 // Set Feed Enable bit
    RAP_write(FEED_CONFIG_1, temp, sensorId);
  }
  else
  {
    temp &= ~0x01;                // Clear Feed Enable bit
    RAP_write(FEED_CONFIG_1, temp, sensorId);
  }
}

// Enables vertical two-finger-scroll gesture (relative-mode only)
void Pinnacle_enableScroll(uint8_t sensorId)
{
  uint8_t temp = 0;
  RAP_readBytes(FEED_CONFIG_2, &temp, 1, sensorId);
  temp &= ~0x08;
  temp |= 0x01;
  RAP_write(FEED_CONFIG_2, temp, sensorId);
}

// Pinnacle includes a feature that allows it to automatically detect when SPI data is
// transitioning on falling edges (rather than rising edges, as it should be). In this case,
// Pinnacle can automatically switch the clock phase of the SPI module to accommodate.
// NOTE: if your SPI signals have excessive ringing and/or glitches, this feature may be triggered
// in error. To disable the feature, call this function immediately after SPI is setup.
void Pinnacle_disableAutoEdgeDetect(uint8_t sensorId)
{
  ERA_writeByte(0xDA, 0x81, sensorId);
}

// Reads XYZ coordinate data from Pinnacle, as well as button states
// NOTE: this function should be called immediately after DR is asserted (HIGH)
void Pinnacle_getAbsolute(touchData_t * touchData, uint8_t sensorId)
{
  uint8_t data[6] = { 0,0,0,0,0,0 };
  RAP_readBytes(PACKET_BYTE_0, data, 6, sensorId);

  Pinnacle_clearFlags(sensorId);

  touchData->absolute.buttons = data[0] & 0x1F;

  touchData->absolute.xValue = data[2] | ((data[4] & 0x0F) << 8);
  touchData->absolute.yValue = data[3] | ((data[4] & 0xF0) << 4);
  touchData->absolute.zValue = data[5] & 0x3F;

  // If the overlay is curved, apply the curved threshold matrix
  if(touchData->overlayMode == CURVED)
  {
      Pinnacle_applyCurvedThresh(&(touchData->absolute));
  }
}

// Reads X, Y, and Scroll-Wheel deltas from Pinnacle, as well as button states
// NOTE: this function should be called immediately after DR is asserted (HIGH)
void Pinnacle_getRelative(touchData_t * touchData, uint8_t sensorId)
{
  uint8_t data[4] = { 0,0,0,0 };
  RAP_readBytes(PACKET_BYTE_0, data, 4, sensorId);

  Pinnacle_clearFlags(sensorId);    // QUEUED

  touchData->relative.buttons = data[0] & 0x07;
  touchData->relative.xDelta = (int8_t)data[1];
  touchData->relative.yDelta = (int8_t)data[2];
  touchData->relative.wheelCount = (int8_t)data[3];
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

void Pinnacle_applyCurvedThresh(absData_t * result)
{
  uint32_t zone_x, zone_y;
  //eliminate hovering
  zone_x = result->xValue / ZONESCALE;
  zone_y = result->yValue / ZONESCALE;
  result->hovering = !(result->zValue > Z_TRESH[zone_y][zone_x]);
}

// Forces Pinnacle to re-calibrate, sometimes useful when miss-compensation
// causes touchpad to miss real touches
void Pinnacle_forceCalibration(uint8_t sensorId)
{
  uint8_t CalConfig1Value = 0x00;

  Pinnacle_enableFeed(false, sensorId);
//  Pinnacle_ClearFlags();
  RAP_readBytes(0x07, &CalConfig1Value, 1, sensorId);
  CalConfig1Value |= 0x01;
  RAP_write(0x07, CalConfig1Value, sensorId);

  do
  {
    RAP_readBytes(0x07, &CalConfig1Value, 1, sensorId);
  }
  while(CalConfig1Value & 0x01);

  Pinnacle_clearFlags(sensorId);
}

// Reads the 46 comp-matrix (uint16_t) values into <*results>.
// NOTE: The meaning of comp-matrix data depends on the electrode configuration
void Pinnacle_getCompMatrix(int16_t * results, uint8_t sensorId)
{
  uint8_t i = 0;
  uint8_t compData[COMP_MATRIX_SIZE];

  ERA_readBytes(COMP_MATRIX_ADDRESS, compData, COMP_MATRIX_SIZE, sensorId); // Get the bytes

  for(; i < COMP_MATRIX_SIZE; i += 2)   // Merge the bytes into int16_t values
  {
    results[i/2] = ((int16_t)compData[i]) << 8;
    results[i/2] |= (int16_t)(compData[i+1]);
  }
}

// Adjusts the feedback in the ADC, effectively attenuating the finger signal
// By default, the the signal is maximally attenuated (ADC_ATTENUATE_4X for use with thin, flat overlays)
// For minimum attenuation, adcGain = ADC_ATTENUATE_1X. See Pinnacle.h for more details.
void Pinnacle_setAdcAttenuation(uint8_t adcGain, uint8_t sensorId)
{
  uint8_t temp = 0x00;

  ERA_readBytes(0x0187, &temp, 1, sensorId);
  temp &= 0x3F; // clear top two bits
  temp |= adcGain;
  ERA_writeByte(0x0187, temp, sensorId);
  ERA_readBytes(0x0187, &temp, 1, sensorId);
}

/*  ERA (Extended Register Access) Functions  */
// Reads <count> bytes from an extended register at <address> (16-bit address),
// stores values in <*data>
void ERA_readBytes(uint16_t address, uint8_t * data, uint16_t count, uint8_t sensorId)
{
  uint8_t ERAControlValue = 0xFF;
  uint16_t i = 0;

  Pinnacle_enableFeed(false, sensorId); // Disable feed

  RAP_write(ERA_HIGH_BYTE, (uint8_t)(address >> 8), sensorId);     // Send upper byte of ERA address
  RAP_write(ERA_LOW_BYTE, (uint8_t)(address & 0x00FF), sensorId); // Send lower byte of ERA address

  for(; i < count; i++)
  {
    RAP_write(ERA_CONTROL, 0x05, sensorId);  // Signal ERA-read (auto-increment) to Pinnacle

    // Wait for status register 0x1E to clear
    do
    {
      RAP_readBytes(ERA_CONTROL, &ERAControlValue, 1, sensorId);
    } while(ERAControlValue != 0x00);

    RAP_readBytes(ERA_VALUE, data + i, 1, sensorId);

    Pinnacle_clearFlags(sensorId);
  }
}

// Writes a byte, <data>, to an extended register at <address> (16-bit address)
void ERA_writeByte(uint16_t address, uint8_t data, uint8_t sensorId)
{
  uint8_t ERAControlValue = 0xFF;

  Pinnacle_enableFeed(false, sensorId); // Disable feed

  RAP_write(ERA_VALUE, data, sensorId);      // Send data byte to be written

  RAP_write(ERA_HIGH_BYTE, (uint8_t)(address >> 8), sensorId);     // Upper byte of ERA address
  RAP_write(ERA_LOW_BYTE, (uint8_t)(address & 0x00FF), sensorId); // Lower byte of ERA address

  RAP_write(ERA_CONTROL, 0x02, sensorId);  // Signal an ERA-write to Pinnacle

  // Wait for status register 0x1E to clear
  do
  {
    RAP_readBytes(ERA_CONTROL, &ERAControlValue, 1, sensorId);
  } while(ERAControlValue != 0x00);

  Pinnacle_clearFlags(sensorId);
}

/* Register Access Protocol (RAP) functions */
// Reads <count> Pinnacle registers starting at <address>
void RAP_readBytes(uint8_t address, uint8_t * data, uint8_t count, uint8_t sensorId)
{
  uint8_t cmdByte = READ_MASK | address;   // Form the READ command byte
  uint8_t i = 0;

  SPI_beginTransaction();

  HW_assertCS(sensorId);
  SPI_transfer(cmdByte);  // Signal a RAP-read operation starting at <address>
  SPI_transfer(0xFC);     // Filler byte
  SPI_transfer(0xFC);     // Filler byte
  for(; i < count; i++)
  {
    data[i] =  SPI_transfer(0xFC);  // Each subsequent SPI transfer gets another register's contents
  }
  HW_deAssertCS(sensorId);

  SPI_endTransaction();
}

// Writes single-byte <data> to <address>
void RAP_write(uint8_t address, uint8_t data, uint8_t sensorId)
{
  uint8_t cmdByte = WRITE_MASK | address;  // Form the WRITE command byte

  SPI_beginTransaction();

  HW_assertCS(sensorId);
  SPI_transfer(cmdByte);  // Signal a write to register at <address>
  SPI_transfer(data);    // Send <value> to be written to register
  HW_deAssertCS(sensorId);

  SPI_endTransaction();
}
