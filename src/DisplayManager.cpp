/**
 * @file DisplayManager.cpp
 * @author Ebrahim Siami
 * @brief OLED UI & Graphics Engine
 * @version 2.1.3
 * @date 2025-01-25
 * 
 * Manages all graphical user interface (GUI) elements including:
 * - Boot splash screen animation
 * - Menu navigation and page rendering
 * - Real-time telemetry display (Battery, Timer, Channels)
 * - Trim visualizers
 * - Advanced Settings Menus (Inversion, Modes)
 * 
 * NEW Features (Update 2.6.1):
 * 
 * - Smart Throttle (Airplane/Quad modes).
 * - Channel Inversion Menu.
 * - Dynamic Refresh Rate.
 * - Loop-decoupled Trim speed.
 */

#include "DisplayManager.h"
#include <Wire.h>

// --- Bitmap Data: MIG-21 Fighter Jet ---
const unsigned char epd_bitmap_mig_21 [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0x00, 0x0e, 0x00, 
	0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xfc, 0x00, 
	0x00, 0x00, 0x03, 0xff, 0xff, 0x83, 0xf8, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 
	0x00, 0xfd, 0xfb, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x00, 0x03, 0xff, 0xff, 0x87, 0xf8, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x01, 0xfc, 0x00, 
	0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x1e, 0x00, 
	0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupDisplay() {
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Halt on failure
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3); 
  
  const char* introText = "Ebr.co";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(introText, 0, 0, &x1, &y1, &w, &h);
  
  int textX = (SCREEN_WIDTH - w) / 2;
  int textY = (SCREEN_HEIGHT - h) / 2;
  
  display.setCursor(textX, textY);
  display.println(introText);

  int padding = 4;
  display.drawRoundRect(textX - padding, textY - padding, w + (2 * padding), h + (2 * padding), 4, SSD1306_WHITE); 

  display.display();
  delay(2000); 
}

void showSavingFeedback() {
  display.clearDisplay();
  display.setTextSize(2); 

  const char* text = "Saving...";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
  display.print(text);
  display.display();
  
  beep(100);
  delay(300); // Wait for EEPROM write cycle
}

void drawBar(const char* label, int x, int y, byte value) {
  display.setCursor(x, y);
  display.print(label);
  int barWidth = 80;
  int filled = map(value, 0, 255, 0, barWidth);
  display.drawRect(x + 30, y, barWidth, 8, SSD1306_WHITE);
  display.fillRect(x + 30, y, filled, 8, SSD1306_WHITE);
}

void showSplashScreen(const char* productName, const unsigned long durationMs) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  const char* smallLogoText = "Ebr.co";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(smallLogoText, 0, 0, &x1, &y1, &w, &h);
  
  const int FINAL_PLANE_X = (SCREEN_WIDTH - 64) / 2;
  const int FINAL_PLANE_Y = (SCREEN_HEIGHT / 2) - 18;
  int loadingBarY = SCREEN_HEIGHT - 10;
  int loadingBarWidth = SCREEN_WIDTH - 20;
  int loadingBarX = 10;
  
  unsigned long startTime = millis();
  unsigned long animationDuration = durationMs / 2;
  if (animationDuration == 0) animationDuration = 1;

  for (unsigned long elapsed = 0; elapsed <= animationDuration; elapsed = millis() - startTime) {
    display.clearDisplay();
    display.setTextSize(1); 
    display.setCursor((SCREEN_WIDTH - w) / 2, 2);
    display.println(smallLogoText);
    
    int currentPlaneX = map(elapsed, 0, animationDuration, SCREEN_WIDTH, FINAL_PLANE_X);
    display.drawBitmap(currentPlaneX, FINAL_PLANE_Y, epd_bitmap_mig_21, 64, 32, SSD1306_WHITE);
    
    int filledWidth = map(elapsed, 0, durationMs, 0, loadingBarWidth);
    display.drawRect(loadingBarX, loadingBarY, loadingBarWidth, 8, SSD1306_WHITE);
    display.fillRect(loadingBarX, loadingBarY, filledWidth, 8, SSD1306_WHITE);
    display.display();
  }
  
  unsigned long currentMillis = millis();
  while (currentMillis < startTime + durationMs) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor((SCREEN_WIDTH - w) / 2, 2);
    display.println(smallLogoText);
    display.drawBitmap(FINAL_PLANE_X, FINAL_PLANE_Y, epd_bitmap_mig_21, 64, 32, SSD1306_WHITE);
    unsigned long elapsed = currentMillis - startTime;
    int filledWidth = map(elapsed, 0, durationMs, 0, loadingBarWidth);
    display.drawRect(loadingBarX, loadingBarY, loadingBarWidth, 8, SSD1306_WHITE);
    display.fillRect(loadingBarX, loadingBarY, filledWidth, 8, SSD1306_WHITE);
    display.display();
    currentMillis = millis();
  }
  display.clearDisplay();
}

void drawCurrentPage(
    DisplayState currentPage,
    int trimsMenuIndex,
    int settingsMenuIndex,
    const RadioSettings& settings,
    byte throttle, byte pitch, byte roll, byte yaw,
    byte aux1, byte aux2, bool aux3, bool aux4,
    float voltage,
    int timerSelection, bool timerIsArmed, bool timerIsRunning, long timerValue, bool isTimeEditMode,
    int invertMenuIndex
) {
  display.clearDisplay(); 
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.invertDisplay(settings.lightModeEnabled);

  String pageDisplayName = ""; 
  int16_t boundX, boundY;
  uint16_t boundW, boundH;
  int navOptionsY = SCREEN_HEIGHT - 9;

  switch (currentPage) {

    // --- PAGE: SYSTEM DASHBOARD ---
    case PAGE_MAIN3: {
        pageDisplayName = "System";

        const float BATT_MAX_VOLTAGE = 8.4;
        const float BATT_MIN_VOLTAGE = 6.0;
        long level = constrain(map(voltage * 100, (long)(BATT_MIN_VOLTAGE * 100), (long)(BATT_MAX_VOLTAGE * 100), 0, 100), 0, 100);
        int battX = 5, battY = 5, battWidth = 28, battHeight = 12;
        display.drawRect(battX, battY, battWidth, battHeight, SSD1306_WHITE);
        display.fillRect(battX + battWidth, battY + 3, 3, battHeight - 6, SSD1306_WHITE);
        int fillWidth = map(level, 0, 100, 0, battWidth - 2);
        if (fillWidth > 0) display.fillRect(battX + 1, battY + 1, fillWidth, battHeight - 2, SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(battX + battWidth + 8, battY + 2);
        display.print(voltage, 2);
        display.print("V");

        display.setCursor(5, 30);
        display.print("TIMER: ");
        char timeText[10];

        if (timerIsArmed || timerIsRunning) {
            long remaining = timerValue;
            if (remaining >= 0) {
                unsigned long minutes = (unsigned long)remaining / 60000;
                unsigned long seconds = ((unsigned long)remaining / 1000) % 60;
                sprintf(timeText, "%02lu:%02lu", minutes, seconds);
            } else {
                display.print("+");
                unsigned long passed = -remaining;
                unsigned long minutes = passed / 60000;
                unsigned long seconds = (passed / 1000) % 60;
                sprintf(timeText, "%02lu:%02lu", minutes, seconds);
            }
        } else {
            if (timerSelection == 0) { sprintf(timeText, "--:--"); }
            else { sprintf(timeText, "%02d:00", timerSelection); }
        }

        if (settingsMenuIndex == 2) {
            display.fillRect(0, 28, SCREEN_WIDTH, 12, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }

        display.setCursor(5, 30);
        display.print("TIMER: ");

        bool shouldShowText = true;
        if (isTimeEditMode && (millis() % 1000 < 500)) {
            shouldShowText = false;
        }

        if (shouldShowText) {
            display.setCursor(5 + 40, 30);
            display.print(timeText);
        }
        
        if (settingsMenuIndex == 0) {
            display.fillRect(SCREEN_WIDTH - 20, navOptionsY, 20, 8, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }
        display.setCursor(SCREEN_WIDTH - 15, navOptionsY);
        display.print(">>");
        break;
    }

    // --- PAGE: CHANNELS 1-4 ---
    case PAGE_MAIN1: {
      drawBar("THT:", 0, 0, throttle);
      drawBar("PIT:", 0, 12, pitch);
      drawBar("ROL:", 0, 24, roll);
      drawBar("YAW:", 0, 36, yaw);

      if (settingsMenuIndex == 0) { 
        display.fillRect(SCREEN_WIDTH - 20, navOptionsY, 20, 8, SSD1306_WHITE); 
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(SCREEN_WIDTH - 15, navOptionsY);
      display.print(">>");

      if (settingsMenuIndex == 1) {
          display.fillRect(0, navOptionsY, 20, 8, SSD1306_WHITE); 
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
          display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(5, navOptionsY);
      display.print("<<");
      pageDisplayName = "Channels 1-4"; 
      break;
    } 

    // --- PAGE: CHANNELS 5-8 ---
    case PAGE_MAIN2: {
      drawBar("AUX1:", 0, 0, aux1);
      drawBar("AUX2:", 0, 12, aux2);
      drawBar("AUX3:", 0, 24, aux3 ? 255 : 0);
      drawBar("AUX4:", 0, 36, aux4 ? 255 : 0);

      if (settingsMenuIndex == 0) {
        display.fillRect(SCREEN_WIDTH - 20, navOptionsY, 20, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(SCREEN_WIDTH - 15, navOptionsY);
      display.print(">>");

      if (settingsMenuIndex == 1) {
        display.fillRect(0, navOptionsY, 20, 8, SSD1306_WHITE); 
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(5, navOptionsY);
      display.print("<<");
      pageDisplayName = "Channels 5-8"; 
      break;
    } 

    // --- PAGE: TRIM ADJUSTMENT ---
    case PAGE_TRIMS: {
      int trimY1 = 3; 
      display.setCursor(0, trimY1); display.print("T1:");
      int lineX1 = 25; int lineY1 = trimY1 + 4; int lineWidth = 70; 
      display.drawLine(lineX1, lineY1, lineX1 + lineWidth, lineY1, SSD1306_WHITE);
      display.drawFastVLine(map(2048, 0, 4095, lineX1, lineX1 + lineWidth), lineY1 - 2, 5, SSD1306_WHITE); 
      display.fillRect(map(settings.trim1, 0, 4095, lineX1, lineX1 + lineWidth) - 1, lineY1 - 3, 3, 7, SSD1306_WHITE);
      display.setCursor(lineX1 + lineWidth + 5, trimY1); 
      display.print(map(settings.trim1, 0, 4095, 0, 100)); display.print("%");

      int trimY2 = 16;
      display.setCursor(0, trimY2); display.print("T2:");
      int lineX2 = 25; int lineY2 = trimY2 + 4;
      display.drawLine(lineX2, lineY2, lineX2 + lineWidth, lineY2, SSD1306_WHITE);
      display.drawFastVLine(map(2048, 0, 4095, lineX2, lineX2 + lineWidth), lineY2 - 2, 5, SSD1306_WHITE);
      display.fillRect(map(settings.trim2, 0, 4095, lineX2, lineX2 + lineWidth) - 1, lineY2 - 3, 3, 7, SSD1306_WHITE);
      display.setCursor(lineX2 + lineWidth + 5, trimY2);
      display.print(map(settings.trim2, 0, 4095, 0, 100)); display.print("%");

      int trimY3 = 29;
      display.setCursor(0, trimY3); display.print("T3:");
      int lineX3 = 25; int lineY3 = trimY3 + 4;
      display.drawLine(lineX3, lineY3, lineX3 + lineWidth, lineY3, SSD1306_WHITE);
      display.drawFastVLine(map(2048, 0, 4095, lineX3, lineX3 + lineWidth), lineY3 - 2, 5, SSD1306_WHITE);
      display.fillRect(map(settings.trim3, 0, 4095, lineX3, lineX3 + lineWidth) - 1, lineY3 - 3, 3, 7, SSD1306_WHITE);
      display.setCursor(lineX3 + lineWidth + 5, trimY3);
      display.print(map(settings.trim3, 0, 4095, 0, 100)); display.print("%");
      
      int resetTrimsY = 42; 
      const char* resetText = "Save Trims"; 
      int16_t rX, rY; uint16_t rW, rH; 
      display.getTextBounds(resetText, 0, 0, &rX, &rY, &rW, &rH); 
      int resetTrimsX = (SCREEN_WIDTH - rW) / 2; 
      display.setCursor(resetTrimsX, resetTrimsY); 

      if (trimsMenuIndex == 0) { 
        display.fillRect(0, resetTrimsY, SCREEN_WIDTH, 8, SSD1306_WHITE); 
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.print(resetText);

      if (trimsMenuIndex == 1) {
        display.fillRect(SCREEN_WIDTH - 20, navOptionsY, 20, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(SCREEN_WIDTH - 15, navOptionsY);
      display.print(">>");

      if (trimsMenuIndex == 2) {
        display.fillRect(0, navOptionsY, 20, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(5, navOptionsY);
      display.print("<<");
      pageDisplayName = "Trim Adjust"; 
      break;
    } 

    // --- PAGE: SETTINGS MENU ---
    case MENU: {
        pageDisplayName = "Settings";
        display.setTextSize(1);
        
        for (int i = 0; i < SETTING_TOTAL - 1; i++) { 
            int y = 8 * i + 4;
            if (i == settingsMenuIndex) {
                display.fillRect(0, y - 1, SCREEN_WIDTH, 9, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            } else {
                display.setTextColor(SSD1306_WHITE);
            }
            
            display.setCursor(5, y);
            switch(i) {
                case SETTING_LIGHT_MODE: 
                    display.print("Light Mode: "); display.print(settings.lightModeEnabled ? "On" : "Off"); break;
                case SETTING_BUZZER: 
                    display.print("Buzzer: "); display.print(settings.buzzerEnabled ? "On" : "Off"); break;                   
                case SETTING_CH_INVERT:
                    display.print("Channel Invert >"); break;
                case SETTING_RESET_TRIMS: 
                    display.print("Reset Trims"); break;
                case SETTING_THROTTLE_MODE: 
                    display.print("Thr Mode: "); 
                    display.print(settings.airplaneMode ? "AIR" : "NRM"); 
                    break;
                case SETTING_INFO: 
                    display.print("About / Info >"); break;
            }
        }
        
        if (settingsMenuIndex == SETTING_BACK) { 
            display.fillRect(0, navOptionsY, 20, 8, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }
        display.setCursor(5, navOptionsY);
        display.print("<<");
        break;
    }

    // --- PAGE: INFO / CREDITS ---
    case PAGE_INFO: {
        pageDisplayName = "Info";
        display.setCursor(0, 5);
        display.println("EBR.co EB_I8L RC");
        display.println("Version 2.6.1");
        display.println("By Ebrahim and Mariya:)");
        display.println();
        display.println("Press Enter to go back.");
        break;
    }

    // --- PAGE: CHANNEL INVERSION ---
    case PAGE_CH_INVERT: {
        pageDisplayName = "Invert Channels";
        display.setTextSize(1);
        
        // Two-column layout: Left (CH1-4), Right (CH5-8)
        for (int i = 0; i < 8; i++) {
            int x = (i < 4) ? 0 : 64;       
            int y = (i % 4) * 10 + 4;       
            
            if (i == invertMenuIndex) {
                display.fillRect(x, y - 1, 60, 9, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            } else {
                display.setTextColor(SSD1306_WHITE);
            }
            
            display.setCursor(x + 2, y);
            display.print("CH"); 
            display.print(i + 1); 
            display.print(":");
            display.print(settings.channelInverted[i] ? "INV" : "NRM");
        }
        
        // Back Button
        int backY = 44;
        if (invertMenuIndex == 8) { 
            display.fillRect(0, backY, SCREEN_WIDTH, 9, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }
        display.setCursor(5, backY + 1);
        display.print("<< BACK");
        
        break;
    }

    case PAGE_CALIBRATION: {
        pageDisplayName = "Calibration"; 
        display.setCursor(10, 20);
        display.setTextSize(2);
        display.print("Coming Soon!");
        break;
    }
  } 

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.getTextBounds(pageDisplayName, 0, 0, &boundX, &boundY, &boundW, &boundH);
  display.setCursor((SCREEN_WIDTH - boundW) / 2, SCREEN_HEIGHT - boundH - 1);
  display.print(pageDisplayName);

  display.display(); 
}