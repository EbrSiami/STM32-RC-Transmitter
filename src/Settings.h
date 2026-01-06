/**
 * @file Settings.h
 * @author Ebrahim Siami
 * @brief EEPROM Data Structure Definition
 * @version 2.6.1
 * @date 2026-01-07
 * 
 * This file defines the 'RadioSettings' structure used to persist 
 * configuration data (trims, channel inversions, UI preferences) 
 * into the external EEPROM.
 */

#pragma once

#include <Arduino.h> // Required for uint8_t types

struct RadioSettings {
    int trim1;                // Trim value for Channel 1 (Roll/Steering)
    int trim2;                // Trim value for Channel 2 (Pitch/Throttle)
    int trim3;                // Trim value for Channel 3 (Yaw/Aux)
    
    bool buzzerEnabled;       // Global buzzer toggle (Mute/Unmute)
    bool lightModeEnabled;    // UI Theme: true = Light Mode, false = Dark Mode
    
    bool channelInverted[8];  // Inversion status for channels 1-8 (true = inverted)
    uint8_t timerProfile;     // Selected timer profile index

    bool airplaneMode; // false = Normal (Center 50%), true = Airplane (Center 0%)
};