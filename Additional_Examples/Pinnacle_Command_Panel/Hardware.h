// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// Designers will need to implement these hardware-specific functions for their own processor/hardware.
// These functions include GPIO access, a delay timer, and a communication peripheral (SPI or I2C)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SPI_MODE0
#define SPI_MODE0 0x00
#endif

#ifndef SPI_MODE1
#define SPI_MODE1 0x04
#endif

#ifndef SPI_MODE2
#define SPI_MODE2 0x08
#endif

#ifndef SPI_MODE3
#define SPI_MODE3 0x0C
#endif

#ifndef LSBFIRST
#define LSBFIRST 0
#endif

#ifndef MSBFIRST
#define MSBFIRST 1
#endif

void HW_init(void);
void HW_assertCS(uint8_t);        // IN PROGRESS
void HW_deAssertCS(uint8_t);      // QUEUED
bool HW_drAsserted(uint8_t);      // QUEUED
void TIMER_delayMicroseconds(uint32_t);

void SPI_init(uint32_t, uint8_t, uint8_t);
void SPI_end(void);
void SPI_beginTransaction(void);
void SPI_endTransaction(void);
uint8_t SPI_transfer(uint8_t);
void SPI_transferBytes(uint8_t *, uint16_t);

#ifdef __cplusplus
}
#endif
