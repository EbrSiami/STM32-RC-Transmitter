/**
 * @file DisplayManager.h
 * @author Ebrahim Siami
 * @brief Interface for OLED Display Management
 * @version 2.1.3
 * @date 2025-01-25

 * Defines display states, menu items, and rendering prototypes.
 */

#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Settings.h" 

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

/**
 * @brief Represents the current active screen/page.
 */
enum DisplayState {
    PAGE_MAIN1,       // Channels 1-4 (Sticks)
    PAGE_MAIN2,       // Channels 5-8 (Aux)
    PAGE_MAIN3,       // System Dashboard (Timer, Battery)
    PAGE_TRIMS,       // Trim Adjustment Page
    MENU,             // Settings Menu
    PAGE_INFO,        // About / Credits
    PAGE_CALIBRATION, // Stick Calibration (WIP)
    PAGE_CH_INVERT    // Channel Inversion (WIP)
};

/**
 * @brief Items available in the Settings Menu.
 */
enum SettingsMenu {
    SETTING_LIGHT_MODE,
    SETTING_BUZZER,
    SETTING_CH_INVERT,
    SETTING_RESET_TRIMS,
    SETTING_CALIBRATION,
    SETTING_INFO,
    SETTING_BACK,     // Navigation Button (<<)
    SETTING_TOTAL     // Sentinel value for loop limits
};

// Global Display Object
extern Adafruit_SSD1306 display;

// Allow display logic to trigger audio feedback
extern void beep(int duration_ms = 100, bool force = false);

// --- Function Prototypes ---

void setupDisplay();
void showSplashScreen(const char* productName, const unsigned long durationMs);
void showSavingFeedback();

/**
 * @brief Renders the entire UI based on the current state.
 * 
 * @param currentPage Current active screen.
 * @param trimsMenuIndex Cursor position in Trim menu.
 * @param settingsMenuIndex Cursor position in Settings menu.
 * @param settings Reference to global settings struct.
 * @param throttle Channel data...
 * @param pitch Channel data...
 * @param roll Channel data...
 * @param yaw Channel data...
 * @param aux1 Channel data...
 * @param aux2 Channel data...
 * @param aux3 Channel data...
 * @param aux4 Channel data...
 * @param voltage Battery voltage in Volts.
 * @param timerSelection Selected timer preset.
 * @param timerIsArmed Is the timer armed?
 * @param timerIsRunning Is the timer currently counting?
 * @param timerValue Remaining time in milliseconds.
 * @param isTimeEditMode Is the user currently editing the timer?
 */
void drawCurrentPage(
    DisplayState currentPage,
    int trimsMenuIndex,
    int settingsMenuIndex,
    const RadioSettings& settings,
    byte throttle, byte pitch, byte roll, byte yaw,
    byte aux1, byte aux2, bool aux3, bool aux4,
    float voltage,
    int timerSelection, bool timerIsArmed, bool timerIsRunning, long timerValue, bool isTimeEditMode
);