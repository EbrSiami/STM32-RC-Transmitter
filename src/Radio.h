/**
 * @file Radio.h
 * @author Ebrahim Siami
 * @brief NRF24L01+ Communication Driver
 * @version 2.1.3
 * @date 2026-01-07
 * 
 * Defines the NRF24L01+ configuration pins and the data packet structure
 * used for communication between the transmitter and receiver.
 * 
 * Fixing Data Packet Structure
 * 
 */

#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <RF24.h>

// --- Hardware Pin Configuration (STM32 BluePill) ---
#define RF_CE_PIN  PB8
#define RF_CSN_PIN PB9

/**
 * @brief Control Data Packet Structure
 * CRITICAL: This structure must match EXACTLY on the Receiver side.
 * Total Size: 14 bytes (6 ints + 2 bools + padding/alignment)
 */
typedef struct {
  uint8_t throttle;
  uint8_t pitch;
  uint8_t roll;
  uint8_t yaw;
  uint8_t aux1;    // Potentiometer / Switch
  uint8_t aux2;    // Potentiometer / Switch
  uint8_t aux3;   // Digital Switch A
  uint8_t aux4;   // Digital Switch B
} data_t;

// Global Radio Object (Defined in Radio.cpp)
extern RF24 radio;

// --- Function Prototypes ---

/**
 * @brief Initializes the radio hardware and settings.
 */
void setupRadio();

/**
 * @brief Sends a single data packet.
 * @param dataToSend The populated data structure.
 */
void sendRadioData(data_t dataToSend);

#endif // RADIO_H