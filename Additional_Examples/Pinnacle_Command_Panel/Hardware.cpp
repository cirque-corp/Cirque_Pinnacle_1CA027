// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <Arduino.h>
#include <SPI.h>
#include "Hardware.h"

#define CS0_PIN   10      // Chip Select pin for Sensor 0
#define DR0_PIN   9       // Data Ready pin for Sensor 0
#define CS1_PIN   8       // Chip Select pin for Sensor 1
#define DR1_PIN   7       // Data Ready pin for Sensor 1

typedef struct _sensorPort
{
  uint8_t CS_Pin;
  uint8_t DR_Pin;
  uint8_t sensorId;     // also the index of the sensor in the code.
} sensorPort_t;

sensorPort_t sensorList[2];

SPISettings _spiSettings;

void HW_init()
{
  // set the CS pins as output and DR pins as inputs
  pinMode(CS0_PIN, OUTPUT);
  pinMode(DR0_PIN, INPUT);
  pinMode(CS1_PIN, OUTPUT);
  pinMode(DR1_PIN, INPUT);

  // create sensorPort objects for the sensorList and insert them into the array
  sensorPort_t temp;

  temp.CS_Pin = CS0_PIN;
  temp.DR_Pin = DR0_PIN;
  temp.sensorId = 0;

  sensorList[0] = temp;

  temp.CS_Pin = CS1_PIN;
  temp.DR_Pin = DR1_PIN;
  temp.sensorId = 1;

  sensorList[1] = temp;
}

void HW_assertCS(uint8_t sensorId)
{
  digitalWriteFast(sensorList[sensorId].CS_Pin, LOW);
}

void HW_deAssertCS(uint8_t sensorId)
{
  digitalWriteFast(sensorList[sensorId].CS_Pin, HIGH);
}

bool HW_drAsserted(uint8_t sensorId)
{
  return digitalRead(sensorList[sensorId].DR_Pin);
}

void TIMER_delayMicroseconds(uint32_t microSeconds)
{
  delayMicroseconds(microSeconds);
}

void SPI_init(uint32_t bitRate, uint8_t bitOrder, uint8_t spiMode)
{
  _spiSettings = SPISettings(bitRate, bitOrder, spiMode);
  SPI.begin();
}

void SPI_end()
{
  SPI.end();
}

void SPI_beginTransaction()
{
  SPI.beginTransaction(_spiSettings);
}

void SPI_endTransaction()
{
  SPI.endTransaction();
}

uint8_t SPI_transfer(uint8_t data)
{
  return SPI.transfer(data);
}

void SPI_transferBytes(uint8_t * data, uint16_t count)
{
  SPI.transfer(data, count);
}
