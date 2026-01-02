/**
 * @file Button.h
 * @author Ebrahim Siami
 * @brief Button Class Interface
 * @version 2.1.3
 * @date 2025-02-06
 * Defines the structure for handling physical buttons with 
 * internal pull-up resistors and software debouncing.
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
public:
  /**
   * @brief Construct a new Button object
   * @param pin The STM32 pin connected to the button.
   * @param debounceDelay Time in ms to wait for signal stabilization (default: 50ms).
   */
  Button(int pin, long debounceDelay = 50);

  /**
   * @brief Configures the pin as INPUT_PULLUP.
   * Must be called in setup().
   */
  void begin();

  /**
   * @brief Reads the button state and handles debouncing logic.
   * CRITICAL: Must be called continuously in the main loop().
   */
  void update();

  /**
   * @brief Checks if the button was pressed and released.
   * @return true only once per click (on release).
   */
  bool wasJustPressed();

  /**
   * @brief Checks if the button is currently held down.
   * @return true while the button is physically pressed (LOW).
   */
  bool isBeingHeld();

private:
  int _pin;
  long _debounceDelay;
  bool _lastPhysicalState;
  bool _buttonIsCurrentlyPressed;
  bool _wasJustPressedFlag;
  unsigned long _lastDebounceTime;
};

#endif