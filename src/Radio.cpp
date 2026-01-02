/**
 * @file Radio.cpp
 * @author Ebrahim Siami
 * @brief NRF24L01+ Communication Driver
 * @version 2.1.3
 * @date 2025-02-01
 * Handles initialization and data transmission using the RF24 library.
 * Configured for 250kbps data rate for maximum range.
 */

#include "Radio.h"
#include <SPI.h>

// Initialize RF24 Object (CE Pin, CSN Pin)
RF24 radio(RF_CE_PIN, RF_CSN_PIN);

// Radio Pipe Address
// This must match the address defined in the receiver code.
const uint64_t pipeOut = 0xE8E8F0F0E1LL;

/**
 * @brief Initializes the NRF24L01 module.
 * Configures Channel 100, 250kbps datarate, and MAX power.
 * Disables AutoAck for lower latency in RC control applications.
 */
void setupRadio() {
  SPI.begin();

  if (!radio.begin()) {
    // Hardware failure handling (optional)
    // Could toggle an LED or show error on OLED if needed.
  }

  radio.openWritingPipe(pipeOut);
  radio.setChannel(100);             // Interference-free channel (usually > WiFi freqs)
  radio.setAutoAck(false);           // Disable Ack for one-way UDP-like protocol
  radio.setDataRate(RF24_250KBPS);   // Longest range setting
  radio.setPALevel(RF24_PA_MAX);     // Maximum power output
  radio.stopListening();             // Transmitter mode only
}

/**
 * @brief Transmits the control data packet.
 * @param dataToSend The structured data packet containing channel values.
 */
void sendRadioData(data_t dataToSend) {
  radio.write(&dataToSend, sizeof(data_t)); 
}