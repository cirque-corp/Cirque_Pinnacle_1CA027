// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifdef __cplusplus
extern "C" {
#endif

// Pinnacle v2.2 Register Access Protocol (RAP) Addresses
#define FIRMWARE_ID       0x00
#define FIRMWARE_VERSION  0x01
#define STATUS_1          0x02
#define SYS_CONFIG_1      0x03
#define FEED_CONFIG_1     0x04
#define FEED_CONFIG_2     0x05
#define FEED_CONFIG_3     0x06
#define CAL_CONFIG_1      0x07
#define PS2_AUX_CONTROL   0x08
#define SAMPLE_RATE       0x09
#define Z_IDLE            0x0A
#define Z_SCALER          0x0B
#define SLEEP_INTERVAL    0x0C  // time of sleep until checking for finger
#define SLEEP_TIMER       0x0D  // time after idle mode until sleep starts
#define RESERVED_0        0x0E
#define RESERVED_1        0x0F
#define RESERVED_2        0x10
#define RESERVED_3        0x11
#define PACKET_BYTE_0     0x12
#define PACKET_BYTE_1     0x13
#define PACKET_BYTE_2     0x14
#define PACKET_BYTE_3     0x15
#define PACKET_BYTE_4     0x16
#define PACKET_BYTE_5     0x17
#define RESERVED_4        0x18  // Port A GPIO Control
#define RESERVED_5        0x19  // Port A GPIO Data
#define RESERVED_6        0x1A  // Port B GPIO Control/Data
#define ERA_VALUE         0x1B
#define ERA_HIGH_BYTE     0x1C
#define ERA_LOW_BYTE      0x1D
#define ERA_CONTROL       0x1E
#define HCO_ID            0x1F

// TM0xx0xx Mapping and Dimensions
#define PINNACLE_XMAX     2047    // max value Pinnacle can report for X (0 to (8 * 256) - 1)
#define PINNACLE_YMAX     1535    // max value Pinnacle can report for Y (0 to (6 * 256) - 1)
#define PINNACLE_X_LOWER  127     // min "reachable" X value
#define PINNACLE_X_UPPER  1919    // max "reachable" X value
#define PINNACLE_Y_LOWER  63      // min "reachable" Y value
#define PINNACLE_Y_UPPER  1471    // max "reachable" Y value
#define PINNACLE_X_RANGE  (PINNACLE_X_UPPER - PINNACLE_X_LOWER)
#define PINNACLE_Y_RANGE  (PINNACLE_Y_UPPER - PINNACLE_Y_LOWER)
#define ZONESCALE 256   // divisor for reducing x,y values to an array index for the LUT
#define ROWS_Y ((PINNACLE_YMAX + 1) / ZONESCALE)
#define COLS_X ((PINNACLE_XMAX + 1) / ZONESCALE)

// ADC-attenuation settings (held in BIT_7 and BIT_6)
// 1X = most sensitive, 4X = least sensitive
#define ADC_ATTENUATE_1X   0x00
#define ADC_ATTENUATE_2X   0x40
#define ADC_ATTENUATE_3X   0x80
#define ADC_ATTENUATE_4X   0xC0

#define RELATIVE  0
#define ABSOLUTE  1

#define FLAT      0
#define CURVED    1

// Custom types make a convenient way to store and access measurements
typedef struct _absData
{
  uint8_t buttons;
  uint16_t xValue;
  uint16_t yValue;
  uint16_t zValue;
  bool hovering;
} absData_t;

typedef struct _relData
{
  uint8_t buttons;
  int8_t xDelta;
  int8_t yDelta;
  int8_t wheelCount;
} relData_t;

typedef struct _touchData
{
  absData_t absolute;
  relData_t relative;
  uint8_t mode;
  uint8_t overlayMode;
} touchData_t;

// Higher-level functions demonstrate usage of Pinnacle
void Pinnacle_init(touchData_t *, uint8_t);
bool Pinnacle_available(uint8_t);
void Pinnacle_cyclePower(uint8_t);
bool Pinnacle_sensorPresent(uint8_t);
void Pinnacle_getTouchData(touchData_t *, uint8_t);
void Pinnacle_applyCurvedThresh(absData_t *);
void Pinnacle_clearFlags(uint8_t);
void Pinnacle_setToAbsolute(touchData_t *, uint8_t);
void Pinnacle_setToRelative(touchData_t *, uint8_t);
void Pinnacle_enableCurved(touchData_t *, bool, uint8_t);
void Pinnacle_setZIdleCount(uint8_t, uint8_t);
void Pinnacle_enableFeed(bool, uint8_t);
void Pinnacle_enableScroll(uint8_t);
void Pinnacle_forceCalibration(uint8_t);
void Pinnacle_getCompMatrix(int16_t *, uint8_t);
bool Pinnacle_sensorPresent(uint8_t);
void Pinnacle_setAdcAttenuation(uint8_t, uint8_t);

// Low-level register access for Pinnacle
void RAP_readBytes(uint8_t, uint8_t *, uint8_t, uint8_t);
void RAP_write(uint8_t, uint8_t, uint8_t);
void ERA_readBytes(uint16_t, uint8_t *, uint16_t, uint8_t);
void ERA_writeByte(uint16_t, uint8_t, uint8_t);

#ifdef __cplusplus
}
#endif
