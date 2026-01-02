/**
 * @file EEPROM_24LCXX.cpp
 * @author Ebrahim Siami
 * @brief Manual I2C Driver for 24LCxx EEPROM Series
 * @version 2.1.3
 * @date 2025-02-13
 * 
 * Provides low-level read/write functions for external EEPROMs
 * connected via I2C2. Handles page-write boundaries and addressing.
 */

#include "EEPROM_24LCXX.h"
#include <Wire.h>

#define EEPROM_I2C_BASE_ADDRESS 0x50

// Use the secondary I2C bus defined in main.cpp
extern TwoWire Wire2;

/**
 * @brief Writes a single byte to a specific EEPROM address.
 */
void EEPROM_writeByte(uint16_t eeAddress, uint8_t data) {
  uint8_t i2cAddr = EEPROM_I2C_BASE_ADDRESS | ((eeAddress >> 8) & 0x03);
  
  Wire2.beginTransmission(i2cAddr);
  Wire2.write((uint8_t)(eeAddress & 0xFF));
  Wire2.write(data);
  Wire2.endTransmission();
  
  delay(5); // Required write cycle time (tWR)
}

/**
 * @brief Reads a single byte from a specific EEPROM address.
 */
uint8_t EEPROM_readByte(uint16_t eeAddress) {
  uint8_t data = 0xFF;
  uint8_t i2cAddr = EEPROM_I2C_BASE_ADDRESS | ((eeAddress >> 8) & 0x03);
  
  Wire2.beginTransmission(i2cAddr);
  Wire2.write((uint8_t)(eeAddress & 0xFF));
  Wire2.endTransmission();
  
  Wire2.requestFrom(i2cAddr, (uint8_t)1);
  if (Wire2.available()) {
    data = Wire2.read();
  }
  return data;
}

/**
 * @brief Writes a buffer of bytes, handling page boundaries automatically.
 * 24LCxx EEPROMs have a page size (usually 16 or 32 bytes). Writing across
 * a page boundary in a single transmission causes data wrap-around.
 * This function splits the write operation to prevent that.
 */
void EEPROM_write(uint16_t eeAddress, const uint8_t* data, size_t len) {
  size_t i = 0;
  while (i < len) {
    uint8_t i2cAddr = EEPROM_I2C_BASE_ADDRESS | ((eeAddress >> 8) & 0x03);
    
    Wire2.beginTransmission(i2cAddr);
    Wire2.write((uint8_t)(eeAddress & 0xFF));

    // Calculate space left in current page (assuming 16-byte pages)
    size_t bytesToWrite = 16 - (eeAddress % 16);
    if (bytesToWrite > (len - i)) {
      bytesToWrite = len - i;
    }

    for (size_t j = 0; j < bytesToWrite; j++) {
      Wire2.write(data[i + j]);
    }

    Wire2.endTransmission();
    delay(5); // Write cycle delay

    eeAddress += bytesToWrite;
    i += bytesToWrite;
  }
}

/**
 * @brief Reads a buffer of bytes sequentially.
 */
void EEPROM_read(uint16_t eeAddress, uint8_t* data, size_t len) {
  size_t i = 0;
  while (i < len) {
    uint8_t i2cAddr = EEPROM_I2C_BASE_ADDRESS | ((eeAddress >> 8) & 0x03);
    
    Wire2.beginTransmission(i2cAddr);
    Wire2.write((uint8_t)(eeAddress & 0xFF));
    Wire2.endTransmission();

    // Limit read chunk size to prevent I2C buffer overflow
    size_t bytesToRead = len - i;
    if (bytesToRead > 16) bytesToRead = 16;

    Wire2.requestFrom(i2cAddr, (uint8_t)bytesToRead);
    for (size_t j = 0; j < bytesToRead; j++) {
      if (Wire2.available()) {
        data[i + j] = Wire2.read();
      } else {
        data[i + j] = 0xFF; // Error fallback
      }
    }

    eeAddress += bytesToRead;
    i += bytesToRead;
  }
}