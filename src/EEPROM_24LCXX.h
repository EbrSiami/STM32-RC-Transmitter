/**
 * @file EEPROM_24LCXX.h
 * @author Ebrahim Siami
 * @brief I2C EEPROM Driver Interface
 * @version 2.1.3
 * @date 2025-02-13
 * 
 * Provides function prototypes for low-level EEPROM access.
 * Compatible with 24LC04, 24LC08, 24LC16, etc.
 */

#ifndef EEPROM_24LCXX_H
#define EEPROM_24LCXX_H

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Writes a single byte to the specified memory address.
 * @param eeAddress Memory location (0 to Device Capacity).
 * @param data Byte to write.
 */
void EEPROM_writeByte(uint16_t eeAddress, uint8_t data);

/**
 * @brief Reads a single byte from the specified memory address.
 * @param eeAddress Memory location.
 * @return The byte read from EEPROM.
 */
uint8_t EEPROM_readByte(uint16_t eeAddress);

/**
 * @brief Writes a block of data to EEPROM.
 * Handles page boundary limits automatically.
 * 
 * @param eeAddress Start address in EEPROM.
 * @param data Pointer to the data buffer.
 * @param len Number of bytes to write.
 */
void EEPROM_write(uint16_t eeAddress, const uint8_t* data, size_t len);

/**
 * @brief Reads a block of data from EEPROM.
 * 
 * @param eeAddress Start address in EEPROM.
 * @param data Pointer to the buffer where data will be stored.
 * @param len Number of bytes to read.
 */
void EEPROM_read(uint16_t eeAddress, uint8_t* data, size_t len);

#endif // EEPROM_24LCXX_H