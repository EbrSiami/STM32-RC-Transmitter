/**
 * @file main.cpp
 * @author Ebrahim Siami
 * @brief STM32 RC Transmitter Firmware - Main Entry Point
 * @version 2.1.3
 * @date 2025-01-25
 * @copyright Copyright (c) 2025
 * Dedicated to Marya for the core logic contributions.
 * 
 * 
 * This file handles the main control loop, including:
 * - Reading analog sticks (ADC) and mapping values.
 * - Managing the UI (OLED Display) and Menu Navigation.
 * - Handling trim adjustments and EEPROM persistence.
 * - Battery monitoring and low-voltage alarms.
 * - Transmitting data via NRF24 module.
 * 
 * Target Board: STM32F103C8 (Blue Pill)
 * Framework: Arduino via PlatformIO
 */

#include <SPI.h>
#include <Wire.h>
#include "Button.h"
#include "Radio.h"
#include "DisplayManager.h" 
#include "I2C_eeprom.h"
#include "Settings.h"

// --- Hardware Configuration ---

// I2C Interface for EEPROM (I2C2 on BluePill)
TwoWire Wire2(PB11, PB10); 
I2C_eeprom eeprom(0x50, 512, &Wire2); 

// Trim Buttons (Active Low)
#define TRIM_BTN_1 PB15
#define TRIM_BTN_2 PA8
#define TRIM_BTN_3 PA9
#define TRIM_BTN_4 PA10
#define TRIM_BTN_5 PA15
#define TRIM_BTN_6 PB3

// Navigation Buttons
#define BTN_ENTER PB12
#define BTN_UP    PB13
#define BTN_DOWN  PB14

// Peripherals
#define BUZZER_PIN PC13
const int VOLTAGE_PIN = PA4;

// --- Constants & Globals ---

// EEPROM Addresses (Base offsets)
#define EEPROM_ADDR_T1  0
#define EEPROM_ADDR_T2  2
#define EEPROM_ADDR_T3  4

extern I2C_eeprom eeprom;
extern RadioSettings settings;

// Button Instances (Pin, Debounce Time)
Button enterButton(BTN_ENTER, 100);
Button upButton(BTN_UP, 100);
Button downButton(BTN_DOWN, 100);

Button trimButton1(TRIM_BTN_1, 50);
Button trimButton2(TRIM_BTN_2, 50);
Button trimButton3(TRIM_BTN_3, 50);
Button trimButton4(TRIM_BTN_4, 50);
Button trimButton5(TRIM_BTN_5, 50);
Button trimButton6(TRIM_BTN_6, 50);

// UI State
DisplayState currentPage = PAGE_MAIN3; 
int trimsMenuIndex = 0;
int settingsMenuIndex = 0;

// Trim Configuration
const int TRIM_STEP = 5;
const int MIN_TRIM_VALUE = 0;
const int MAX_TRIM_VALUE = 4095;

// Battery Monitor Configuration
const float R1 = 22.0; // Voltage Divider Resistor 1
const float R2 = 6.8;  // Voltage Divider Resistor 2
const float ADC_MAX_VOLTAGE = 3.3;
const float CORRECTION_FACTOR = 1.01;
const float LOW_BATT_WARNING_VOLTAGE = 6.4; // ~3.2V per cell (2S LiPo)
float batteryVoltage = 0.0;
bool lowBatteryWarningActive = false;

// Buzzer State Machine
unsigned long lastBeepTime = 0;
int beepPhase = 0; // 0=off, 1=beep1, 2=pause, 3=beep2...

// Timer System
unsigned long timerStartMillis = 0;
unsigned long countdownStartMillis = 0;
int selectedTimerMinutes = 0; // 0=Off, 2, 5, 10
bool isTimerArmed = false;
bool isTimerRunning = false;
long timerRemainingMillis = 0;
bool isTimeEditMode = false;

// Radio State
RadioSettings settings;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 10; // 10ms update rate (~100Hz)
data_t data; // Signal data packet

// --- Helper Functions ---

/**
 * @brief Resets control data to safe center/default values.
 */
void ResetData() {
    data.throttle = 0;
    data.pitch    = 128; // Center
    data.roll     = 128; // Center
    data.yaw      = 128; // Center
    data.aux1     = 128;
    data.aux2     = 128;
    data.aux3     = false; 
    data.aux4     = false; 
}

/**
 * @brief Maps raw ADC values to 0-255 byte range based on trim settings.
 * Handles split mapping (Lower->Center and Center->Upper) separately.
 */
int Border_Map(int val, int lower, int middle, int upper, bool reverse) {
    val = constrain(val, lower, upper); 
    if (val < middle)
        val = map(val, lower, middle, 0, 128); 
    else
        val = map(val, middle, upper, 128, 255); 
    return (reverse ? 255 - val : val); 
}

void setupBuzzer() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void beep(int duration_ms, bool force) {
    if (!force && !settings.buzzerEnabled) {
        return;
    }
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration_ms);
    digitalWrite(BUZZER_PIN, LOW);
}

/**
 * @brief Non-blocking state machine for low battery alarm pattern.
 * Pattern: Beep -> Pause -> Beep -> Long Pause.
 */
void handleLowBatteryAlarm() {
    if (batteryVoltage < LOW_BATT_WARNING_VOLTAGE && batteryVoltage > 4.0) {
        lowBatteryWarningActive = true;
    } else {
        lowBatteryWarningActive = false;
        digitalWrite(BUZZER_PIN, LOW);
        beepPhase = 0;
        return;
    }

    unsigned long currentTime = millis();

    switch (beepPhase) {
        case 0: // Start first beep
            digitalWrite(BUZZER_PIN, HIGH);
            lastBeepTime = currentTime;
            beepPhase = 1;
            break;
        case 1: // End first beep (150ms)
            if (currentTime - lastBeepTime >= 150) {
                digitalWrite(BUZZER_PIN, LOW);
                lastBeepTime = currentTime;
                beepPhase = 2;
            }
            break;
        case 2: // Short pause (50ms)
            if (currentTime - lastBeepTime >= 50) {
                lastBeepTime = currentTime;
                beepPhase = 3;
            }
            break;
        case 3: // Start second beep
            digitalWrite(BUZZER_PIN, HIGH);
            lastBeepTime = currentTime;
            beepPhase = 4;
            break;
        case 4: // End second beep (150ms)
            if (currentTime - lastBeepTime >= 150) {
                digitalWrite(BUZZER_PIN, LOW);
                lastBeepTime = currentTime;
                beepPhase = 5;
            }
            break;
        case 5: // Long pause (1000ms)
            if (currentTime - lastBeepTime >= 1000) {
                beepPhase = 0; // Restart cycle
            }
            break;
    }
}

void saveSettings() {
    eeprom.writeBlock(0, (byte*)&settings, sizeof(settings));
}

/**
 * @brief Loads settings from EEPROM and validates data integrity.
 * Loads defaults if EEPROM is empty or corrupted.
 */
void loadSettings() {
    eeprom.readBlock(0, (byte*)&settings, sizeof(settings));

    // Validation Check: If trim is out of bounds, data is likely corrupt/new.
    if (settings.trim1 < MIN_TRIM_VALUE || settings.trim1 > MAX_TRIM_VALUE || isnan(settings.trim1)) {
        settings.trim1 = 2048;
        settings.trim2 = 2048;
        settings.trim3 = 2048;
        settings.buzzerEnabled = true;
        settings.lightModeEnabled = false;
        for (int i = 0; i < 8; i++) {
            settings.channelInverted[i] = false;
        }
        
        // Force write defaults
        eeprom.writeBlock(0, (byte*)&settings, sizeof(settings));
    }
}

void handleCountdownTimer() {
    if (!isTimerRunning) return;

    static unsigned long previousBeepTime = 0;
    unsigned long currentMillis = millis();

    // Beep every second
    if (currentMillis - previousBeepTime >= 1000) {
        previousBeepTime = currentMillis;
        beep(50, true);
    }
}

void handleTimerLogic() {
    if (!isTimerRunning) return;

    unsigned long totalTimerDuration = selectedTimerMinutes * 60000UL;
    unsigned long elapsedTime = millis() - countdownStartMillis;

    if (elapsedTime >= totalTimerDuration) {
        // Timer Finished
        timerRemainingMillis = 0;
        isTimerRunning = false;
        isTimerArmed = false;
        
        beep(500, true); 
        beep(500, true);
    } else {
        timerRemainingMillis = totalTimerDuration - elapsedTime;
    }
}

void handleNavigationButtons() {
    int currentMaxIndex = 0;

    // Define Menu Depths based on pages
    if (currentPage == PAGE_MAIN3) currentMaxIndex = 2;
    else if (currentPage == PAGE_MAIN1 || currentPage == PAGE_MAIN2) currentMaxIndex = 1;
    else if (currentPage == PAGE_TRIMS) currentMaxIndex = 2;
    else if (currentPage == MENU) currentMaxIndex = SETTING_TOTAL - 1;

    // --- UP BUTTON ---
    if (upButton.wasJustPressed()) {
        beep(40, false);
        if (isTimeEditMode) {
            const int times[] = {0, 2, 5, 10};
            int currentIndex = 0;
            for(int i=0; i<4; i++) { if(times[i] == selectedTimerMinutes) { currentIndex = i; break; } }
            currentIndex = (currentIndex + 1) % 4;
            selectedTimerMinutes = times[currentIndex];
        } else {
            if (currentPage == PAGE_TRIMS) {
                trimsMenuIndex = (trimsMenuIndex - 1 + 3) % 3;
            } else {
                settingsMenuIndex = (settingsMenuIndex - 1 + (currentMaxIndex + 1)) % (currentMaxIndex + 1);
            }
        }
    }

    // --- DOWN BUTTON ---
    if (downButton.wasJustPressed()) {
        beep(40, false);
        if (isTimeEditMode) {
            const int times[] = {0, 2, 5, 10};
            int currentIndex = 0;
            for(int i=0; i<4; i++) { if(times[i] == selectedTimerMinutes) { currentIndex = i; break; } }
            currentIndex = (currentIndex - 1 + 4) % 4;
            selectedTimerMinutes = times[currentIndex];
        } else {
            if (currentPage == PAGE_TRIMS) {
                trimsMenuIndex = (trimsMenuIndex + 1) % 3;
            } else {
                settingsMenuIndex = (settingsMenuIndex + 1) % (currentMaxIndex + 1);
            }
        }
    }

    // --- ENTER BUTTON ---
    if (enterButton.wasJustPressed()) {
        beep(50, false);
        if (currentPage == PAGE_MAIN3) {
            if (settingsMenuIndex == 2) { // Timer Selection
                if (isTimeEditMode) {
                    isTimeEditMode = false;
                    if (selectedTimerMinutes > 0) {
                        isTimerArmed = true;
                        isTimerRunning = true;
                        countdownStartMillis = millis();
                    } else {
                        isTimerArmed = false;
                        isTimerRunning = false;
                    }
                    beep(100, false);
                } else {
                    isTimeEditMode = true;
                    isTimerArmed = false;
                    isTimerRunning = false;
                    beep(100, false);
                }
            } else if (settingsMenuIndex == 0) {
                currentPage = PAGE_MAIN1; settingsMenuIndex = 0;
            }
        }
        else if (currentPage == PAGE_MAIN1) {
            if (settingsMenuIndex == 0) { currentPage = PAGE_MAIN2; settingsMenuIndex = 0; }
            else if (settingsMenuIndex == 1) { currentPage = PAGE_MAIN3; settingsMenuIndex = 0; }
        } 
        else if (currentPage == PAGE_MAIN2) {
            if (settingsMenuIndex == 0) { currentPage = PAGE_TRIMS; trimsMenuIndex = 0; }
            else if (settingsMenuIndex == 1) { currentPage = PAGE_MAIN1; settingsMenuIndex = 0; }
        } 
        else if (currentPage == PAGE_TRIMS) {
            if (trimsMenuIndex == 0) { saveSettings(); showSavingFeedback(); }
            else if (trimsMenuIndex == 1) { currentPage = MENU; settingsMenuIndex = 0; }
            else if (trimsMenuIndex == 2) { currentPage = PAGE_MAIN2; settingsMenuIndex = 0; }
        } 
        else if (currentPage == MENU) {
            switch (settingsMenuIndex) {
                case SETTING_BACK: currentPage = PAGE_TRIMS; trimsMenuIndex = 0; beep(100, false); break;
                case SETTING_LIGHT_MODE: settings.lightModeEnabled = !settings.lightModeEnabled; saveSettings(); showSavingFeedback(); break;
                case SETTING_BUZZER: settings.buzzerEnabled = !settings.buzzerEnabled; saveSettings(); showSavingFeedback(); break;
                case SETTING_CH_INVERT: currentPage = PAGE_CH_INVERT; break;
                case SETTING_RESET_TRIMS: settings.trim1 = 2048; settings.trim2 = 2048; settings.trim3 = 2048; saveSettings(); showSavingFeedback(); break;
                case SETTING_INFO: currentPage = PAGE_INFO; break;
                case SETTING_CALIBRATION: currentPage = PAGE_CALIBRATION; break;
            }
        } 
        else if (currentPage == PAGE_INFO || currentPage == PAGE_CALIBRATION || currentPage == PAGE_CH_INVERT) {
            currentPage = MENU;
        }
    }
}

void handleTrimButtons() {
    // Trim 1
    if (trimButton1.wasJustPressed()) beep(20);
    if (trimButton1.isBeingHeld()) {
        if (settings.trim1 < MAX_TRIM_VALUE) settings.trim1 += TRIM_STEP;
    }
    if (trimButton2.wasJustPressed()) beep(20);
    if (trimButton2.isBeingHeld()) {
        if (settings.trim1 > MIN_TRIM_VALUE) settings.trim1 -= TRIM_STEP;
    }

    // Trim 2
    if (trimButton3.wasJustPressed()) beep(20);
    if (trimButton3.isBeingHeld()) {
        if (settings.trim2 < MAX_TRIM_VALUE) settings.trim2 += TRIM_STEP;
    }
    if (trimButton4.wasJustPressed()) beep(20);
    if (trimButton4.isBeingHeld()) {
        if (settings.trim2 > MIN_TRIM_VALUE) settings.trim2 -= TRIM_STEP;
    }

    // Trim 3
    if (trimButton5.wasJustPressed()) beep(20);
    if (trimButton5.isBeingHeld()) {
        if (settings.trim3 < MAX_TRIM_VALUE) settings.trim3 += TRIM_STEP;
    }
    if (trimButton6.wasJustPressed()) beep(20);
    if (trimButton6.isBeingHeld()) {
        if (settings.trim3 > MIN_TRIM_VALUE) settings.trim3 -= TRIM_STEP;
    }
}

// --- Main Setup ---
void setup() {
    pinMode(VOLTAGE_PIN, INPUT_ANALOG);
    timerStartMillis = millis();

    analogReadResolution(12); // STM32 ADC is 12-bit
    setupBuzzer();

    // Initialize Inputs
    enterButton.begin();
    upButton.begin();
    downButton.begin();
    trimButton1.begin();
    trimButton2.begin();
    trimButton3.begin();
    trimButton4.begin();
    trimButton5.begin();
    trimButton6.begin();

    // Initialize I2C
    Wire.begin();   // OLED
    Wire2.begin();  // EEPROM

    delay(500); // Hardware stabilization delay

    setupDisplay();
    showSplashScreen("System Init...", 3000);

    setupRadio();
    loadSettings();
    ResetData(); 
}

// --- Main Loop ---
void loop() {
    // 1. Update Input States
    enterButton.update();
    upButton.update();
    downButton.update();
    trimButton1.update();
    trimButton2.update();
    trimButton3.update();
    trimButton4.update();
    trimButton5.update();
    trimButton6.update();

    // 2. Battery Monitoring
    int adcValue = analogRead(VOLTAGE_PIN);
    if (adcValue > 100) { 
         const float VOLTAGE_CONVERSION_FACTOR = (ADC_MAX_VOLTAGE / 4095.0) * ((R1 + R2) / R2) * CORRECTION_FACTOR;
         batteryVoltage = adcValue * VOLTAGE_CONVERSION_FACTOR;
    }
    handleLowBatteryAlarm();

    // 3. User Interface Logic
    handleTrimButtons();
    handleNavigationButtons(); 
    handleCountdownTimer();
    handleTimerLogic();

    // 4. Map Inputs to Control Data
    data.roll       = Border_Map(analogRead(PA0), 0, settings.trim1, 4095, true);
    data.pitch      = Border_Map(analogRead(PA1), 0, settings.trim2, 4095, true);
    data.throttle   = Border_Map(analogRead(PA2), 0, 2047, 4095, false); 
    data.yaw        = Border_Map(analogRead(PA3), 0, settings.trim3, 4095, true);
    data.aux1       = Border_Map(analogRead(PB0), 0, 2048, 4095, true); 
    data.aux2       = Border_Map(analogRead(PB1), 0, 2048, 4095, true);
    data.aux3       = digitalRead(PB4);
    data.aux4       = digitalRead(PB5);

    // 5. Radio Transmission
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= SEND_INTERVAL) {
        lastSendTime = currentTime; 
        sendRadioData(data); 
    }

    // 6. Update Display
    drawCurrentPage(currentPage, trimsMenuIndex, settingsMenuIndex,
                    settings,
                    data.throttle, data.pitch, data.roll, data.yaw,
                    data.aux1, data.aux2, data.aux3, data.aux4,
                    batteryVoltage,
                    selectedTimerMinutes, isTimerArmed, isTimerRunning, timerRemainingMillis, isTimeEditMode
                    );
}