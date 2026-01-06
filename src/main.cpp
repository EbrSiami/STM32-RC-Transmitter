/**
 * @file main.cpp
 * @author Ebrahim Siami
 * @brief STM32 RC Transmitter Firmware - Main Entry Point
 * @version 2.6.1
 * @date 2026-01-07
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
 * NEW Features (Update 2.6.1):
 * 
 * - Smart Throttle (Airplane/Quad modes).
 * - Channel Inversion Menu.
 * - Dynamic Refresh Rate.
 * - Loop-decoupled Trim speed.
 * 
 * Target Board: STM32F103C8 (Blue Pill)
 * Framework: Arduino via PlatformIO
 */

// Btw Jews Won't Allows me to save Throttle Mode in EEPROM.

#include <SPI.h>
#include <Wire.h>
#include "Button.h"
#include "Radio.h"
#include "DisplayManager.h" 
#include "I2C_eeprom.h"
#include "Settings.h"

// --- Hardware Configuration ---
TwoWire Wire2(PB11, PB10); // I2C2 for EEPROM
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

// --- Globals ---
extern I2C_eeprom eeprom;
extern RadioSettings settings;

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
int invertMenuIndex = 0;

// Trim Config
const int TRIM_STEP = 5;
const int MIN_TRIM_VALUE = 0;
const int MAX_TRIM_VALUE = 4095;

// Battery Monitor
const float R1 = 22.0;
const float R2 = 6.8;
const float ADC_MAX_VOLTAGE = 3.3;
const float CORRECTION_FACTOR = 1.125;
const float LOW_BATT_WARNING_VOLTAGE = 6.4; 
float batteryVoltage = 0.0;
bool lowBatteryWarningActive = false;

// System State
unsigned long lastBeepTime = 0;
int beepPhase = 0;

// Timer
unsigned long timerStartMillis = 0;
unsigned long countdownStartMillis = 0;
int selectedTimerMinutes = 0;
bool isTimerArmed = false;
bool isTimerRunning = false;
long timerRemainingMillis = 0;
bool isTimeEditMode = false;

// Radio & Telemetry
RadioSettings settings;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 4; // 250Hz Update Rate
data_t data;

// Display Refresh Logic
unsigned long lastDisplayTime = 0;

// --- Helper Functions ---

void ResetData() {
    data.throttle = 0;
    data.pitch    = 128;
    data.roll     = 128;
    data.yaw      = 128;
    data.aux1     = 128;
    data.aux2     = 128;
    data.aux3     = false; 
    data.aux4     = false; 
}

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
    if (!force && !settings.buzzerEnabled) return;
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration_ms);
    digitalWrite(BUZZER_PIN, LOW);
}

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
        case 0: digitalWrite(BUZZER_PIN, HIGH); lastBeepTime = currentTime; beepPhase = 1; break;
        case 1: if (currentTime - lastBeepTime >= 150) { digitalWrite(BUZZER_PIN, LOW); lastBeepTime = currentTime; beepPhase = 2; } break;
        case 2: if (currentTime - lastBeepTime >= 50) { lastBeepTime = currentTime; beepPhase = 3; } break;
        case 3: digitalWrite(BUZZER_PIN, HIGH); lastBeepTime = currentTime; beepPhase = 4; break;
        case 4: if (currentTime - lastBeepTime >= 150) { digitalWrite(BUZZER_PIN, LOW); lastBeepTime = currentTime; beepPhase = 5; } break;
        case 5: if (currentTime - lastBeepTime >= 1000) { beepPhase = 0; } break;
    }
}

void saveSettings() {
    eeprom.writeBlock(0, (byte*)&settings, sizeof(settings));
}

void loadSettings() {
    eeprom.readBlock(0, (byte*)&settings, sizeof(settings));

    // Validate Integrity
    if (settings.trim1 < MIN_TRIM_VALUE || settings.trim1 > MAX_TRIM_VALUE || isnan(settings.trim1)) {
        settings.trim1 = 2048;
        settings.trim2 = 2048;
        settings.trim3 = 2048;
        settings.airplaneMode = false;
        settings.buzzerEnabled = true;
        settings.lightModeEnabled = false;
        for (int i = 0; i < 8; i++) {
            settings.channelInverted[i] = false;
        }
        eeprom.writeBlock(0, (byte*)&settings, sizeof(settings));
    }
}

void handleCountdownTimer() {
    if (!isTimerRunning) return;
}

void handleTimerLogic() {
    if (!isTimerRunning) return;
    unsigned long totalTimerDuration = selectedTimerMinutes * 60000UL;
    unsigned long elapsedTime = millis() - countdownStartMillis;

    if (elapsedTime >= totalTimerDuration) {
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

    if (currentPage == PAGE_MAIN3) currentMaxIndex = 2;
    else if (currentPage == PAGE_MAIN1 || currentPage == PAGE_MAIN2) currentMaxIndex = 1;
    else if (currentPage == PAGE_TRIMS) currentMaxIndex = 2;
    else if (currentPage == MENU) currentMaxIndex = SETTING_TOTAL - 1;
    else if (currentPage == PAGE_CH_INVERT) currentMaxIndex = 8; 

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
            } else if (currentPage == PAGE_CH_INVERT) {
                invertMenuIndex = (invertMenuIndex - 1 + (currentMaxIndex + 1)) % (currentMaxIndex + 1);
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
            } else if (currentPage == PAGE_CH_INVERT) {
                invertMenuIndex = (invertMenuIndex + 1) % (currentMaxIndex + 1);
            } else {
                settingsMenuIndex = (settingsMenuIndex + 1) % (currentMaxIndex + 1);
            }
        }
    }

    // --- ENTER BUTTON ---
    if (enterButton.wasJustPressed()) {
        beep(50, false);
        
        if (currentPage == PAGE_MAIN3) {
            if (settingsMenuIndex == 2) { 
                if (isTimeEditMode) {
                    isTimeEditMode = false;
                    if (selectedTimerMinutes > 0) {
                        isTimerArmed = true; isTimerRunning = true; countdownStartMillis = millis();
                    } else {
                        isTimerArmed = false; isTimerRunning = false;
                    }
                    beep(100, false);
                } else {
                    isTimeEditMode = true; isTimerArmed = false; isTimerRunning = false; beep(100, false);
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
                
                // Important: Reset index when entering Invert page
                case SETTING_CH_INVERT: currentPage = PAGE_CH_INVERT; invertMenuIndex = 0; break;
                
                case SETTING_RESET_TRIMS: settings.trim1 = 2048; settings.trim2 = 2048; settings.trim3 = 2048; saveSettings(); showSavingFeedback(); break;
                case SETTING_THROTTLE_MODE: 
                    settings.airplaneMode = !settings.airplaneMode; 
                    saveSettings(); showSavingFeedback(); 
                    break;
                case SETTING_INFO: currentPage = PAGE_INFO; break;
            }
        } 
        
        // --- Dedicated Invert Page Logic ---
        else if (currentPage == PAGE_CH_INVERT) {
            if (invertMenuIndex == 8) { // Back Option
                currentPage = MENU;
                settingsMenuIndex = SETTING_CH_INVERT; 
            } else {
                // Toggle Channel
                settings.channelInverted[invertMenuIndex] = !settings.channelInverted[invertMenuIndex];
                saveSettings(); 
                beep(100, false);
            }
        }
        else if (currentPage == PAGE_INFO || currentPage == PAGE_CALIBRATION) {
            currentPage = MENU;
        }
    }
}

void handleTrimButtons() {
    static unsigned long lastTrimTime = 0;
    const unsigned long TRIM_SPEED_DELAY = 50; 
    bool canAdjust = (millis() - lastTrimTime >= TRIM_SPEED_DELAY);

    // --- Trim 1 ---
    if (trimButton1.wasJustPressed()) beep(20);
    if (trimButton1.isBeingHeld() && canAdjust) {
        if (settings.trim1 < MAX_TRIM_VALUE) { settings.trim1 += TRIM_STEP; lastTrimTime = millis(); }
    }
    if (trimButton2.wasJustPressed()) beep(20);
    if (trimButton2.isBeingHeld() && canAdjust) {
        if (settings.trim1 > MIN_TRIM_VALUE) { settings.trim1 -= TRIM_STEP; lastTrimTime = millis(); }
    }

    // --- Trim 2 ---
    if (trimButton3.wasJustPressed()) beep(20);
    if (trimButton3.isBeingHeld() && canAdjust) {
        if (settings.trim2 < MAX_TRIM_VALUE) { settings.trim2 += TRIM_STEP; lastTrimTime = millis(); }
    }
    if (trimButton4.wasJustPressed()) beep(20);
    if (trimButton4.isBeingHeld() && canAdjust) {
        if (settings.trim2 > MIN_TRIM_VALUE) { settings.trim2 -= TRIM_STEP; lastTrimTime = millis(); }
    }

    // --- Trim 3 ---
    if (trimButton5.wasJustPressed()) beep(20);
    if (trimButton5.isBeingHeld() && canAdjust) {
        if (settings.trim3 < MAX_TRIM_VALUE) { settings.trim3 += TRIM_STEP; lastTrimTime = millis(); }
    }
    if (trimButton6.wasJustPressed()) beep(20);
    if (trimButton6.isBeingHeld() && canAdjust) {
        if (settings.trim3 > MIN_TRIM_VALUE) { settings.trim3 -= TRIM_STEP; lastTrimTime = millis(); }
    }
}

// --- Main Setup ---
void setup() {
    pinMode(VOLTAGE_PIN, INPUT_ANALOG);
    timerStartMillis = millis();

    analogReadResolution(12);
    setupBuzzer();

    enterButton.begin(); upButton.begin(); downButton.begin();
    trimButton1.begin(); trimButton2.begin(); trimButton3.begin();
    trimButton4.begin(); trimButton5.begin(); trimButton6.begin();

    Wire.begin();   // OLED
    Wire2.begin();  // EEPROM
    Wire.setClock(400000); 

    delay(500); 

    setupDisplay();
    showSplashScreen("System Init...", 3000);

    setupRadio();
    loadSettings();
    settings.airplaneMode = true; // User preference override
    ResetData(); 
}

// --- Main Loop ---
void loop() {
    // 1. Inputs
    enterButton.update(); upButton.update(); downButton.update();
    trimButton1.update(); trimButton2.update(); trimButton3.update();
    trimButton4.update(); trimButton5.update(); trimButton6.update();

    // 2. Battery
    int adcValue = analogRead(VOLTAGE_PIN);
    if (adcValue > 100) { 
         const float VOLTAGE_CONVERSION_FACTOR = (ADC_MAX_VOLTAGE / 4095.0) * ((R1 + R2) / R2) * CORRECTION_FACTOR;
         batteryVoltage = adcValue * VOLTAGE_CONVERSION_FACTOR;
    }
    handleLowBatteryAlarm();

    // 3. UI Logic
    handleTrimButtons();
    handleNavigationButtons(); 
    handleCountdownTimer();
    handleTimerLogic();

    // 4. Input Mapping
    data.roll       = Border_Map(analogRead(PA0), 0, settings.trim1, 4095, true  ^ settings.channelInverted[0]);
    data.pitch      = Border_Map(analogRead(PA1), 0, settings.trim2, 4095, true  ^ settings.channelInverted[1]);
    data.yaw        = Border_Map(analogRead(PA3), 0, settings.trim3, 4095, true  ^ settings.channelInverted[3]);
    data.aux1       = Border_Map(analogRead(PB0), 0, 2048, 4095,           true  ^ settings.channelInverted[4]); 
    data.aux2       = Border_Map(analogRead(PB1), 0, 2048, 4095,           true  ^ settings.channelInverted[5]);

    // Throttle Logic
    int rawThrottle = analogRead(PA2);
    uint8_t mappedThrottle = 0;

    if (settings.airplaneMode) {
        if (rawThrottle < 2048) {
            mappedThrottle = 0;
        } else {
            mappedThrottle = map(rawThrottle, 2048, 4095, 0, 255);
        }
    } else {
        mappedThrottle = Border_Map(rawThrottle, 0, 2047, 4095, false);
    }

    if (settings.channelInverted[2]) { 
        data.throttle = 255 - mappedThrottle;
    } else {
        data.throttle = mappedThrottle;
    }

    // Switch Inversion
    bool aux3Raw = digitalRead(PB4);
    data.aux3 = settings.channelInverted[6] ? !aux3Raw : aux3Raw;
    
    bool aux4Raw = digitalRead(PB5);
    data.aux4 = settings.channelInverted[7] ? !aux4Raw : aux4Raw;

    // 5. Radio
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= SEND_INTERVAL) {
        lastSendTime = currentTime; 
        sendRadioData(data); 
    }

    // 6. Display
    unsigned long dynamicInterval = (currentPage == PAGE_MAIN3) ? 100 : 40;

    if (currentTime - lastDisplayTime >= dynamicInterval) {
        lastDisplayTime = currentTime;
        
        drawCurrentPage(currentPage, trimsMenuIndex, settingsMenuIndex,
                    settings,
                    data.throttle, data.pitch, data.roll, data.yaw,
                    data.aux1, data.aux2, data.aux3, data.aux4,
                    batteryVoltage,
                    selectedTimerMinutes, isTimerArmed, isTimerRunning, timerRemainingMillis, isTimeEditMode,
                    invertMenuIndex
                    );
    }
}