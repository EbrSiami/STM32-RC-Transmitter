/**
 * @file DisplayManager.h
 * @author Ebrahim Siami
 * @brief Interface for OLED Display Management
 * @version 2.6.1
 * @date 2026-01-06

 * Defines display states, menu items, and rendering prototypes.
 *
 * NEW Features (Update 2.6.1):
 * 
 * - Smart Throttle (Airplane/Quad modes).
 * - Channel Inversion Menu.
 * - Dynamic Refresh Rate.
 * - Loop-decoupled Trim speed.
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
    PAGE_CH_INVERT    // Channel Inversion
};

/**
 * @brief Items available in the Settings Menu.
 * Order must match the switch-case loop in DisplayManager.cpp
 */
enum SettingsMenu {
    SETTING_LIGHT_MODE,
    SETTING_BUZZER,
    SETTING_CH_INVERT,
    SETTING_RESET_TRIMS,
    SETTING_THROTTLE_MODE, // <--- New Feature
    SETTING_INFO,
    SETTING_BACK,          // Navigation Button (<<)
    SETTING_TOTAL          // Sentinel value
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
 * Updated signature to include Invert Menu cursor.
 */
void drawCurrentPage(
    DisplayState currentPage,
    int trimsMenuIndex,
    int settingsMenuIndex,
    const RadioSettings& settings,
    byte throttle, byte pitch, byte roll, byte yaw,
    byte aux1, byte aux2, bool aux3, bool aux4,
    float voltage,
    int timerSelection, bool timerIsArmed, bool timerIsRunning, long timerValue, bool isTimeEditMode,
    int invertMenuIndex // <--- New Argument
);