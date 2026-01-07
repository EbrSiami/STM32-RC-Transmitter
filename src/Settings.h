/**
 * @file Settings.h
 * @author Ebrahim Siami
 * @brief EEPROM Data Structure Definition
 * @version 2.6.1
 * @date 2026-01-07
 *
 * Description:
 * Defines the 'RadioSettings' structure used to persist configuration data
 * (trims, channel inversions, UI preferences) into the external EEPROM.
 * 
 * WARNING: Changing the order or type of members in this struct will
 * invalidate existing EEPROM data!
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

struct RadioSettings {
    // --- Trim Configuration ---
    // Range: 0 - 4095 (Center: 2048)
    int trim1;                // Channel 1 (Roll/Aileron)
    int trim2;                // Channel 2 (Pitch/Elevator)
    int trim3;                // Channel 3 (Yaw/Rudder)
    
    // --- UI Preferences ---
    bool buzzerEnabled;       // true = Sound On, false = Mute
    bool lightModeEnabled;    // true = Light Background, false = Dark Background
    
    // --- Channel Logic ---
    bool channelInverted[8];  // Inversion map for CH1-CH8 (true = Inverted)
    uint8_t timerProfile;     // Saved index of the selected timer duration

    // --- Flight Mode Configuration ---
    // false = Normal/Quad Mode (Standard linear mapping)
    // true  = Airplane Mode (Specific throttle curve/cutoff)
    bool airplaneMode; 
};

#endif // SETTINGS_H