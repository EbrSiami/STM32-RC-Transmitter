/**
 * @file Button.cpp
 * @author Ebrahim Siami
 * @brief Non-blocking Switch Debouncing Implementation
 * @version 2.1.3
 * @date 2025-02-05
 * 
 * Handles push-button logic with software debouncing.
 * Configured for Active-Low wiring (Internal Pull-up).
 * Triggers 'press' events on button release.
 */

#include "Button.h"
#include <Arduino.h>

Button::Button(int pin, long debounceDelay) :
  _pin(pin),
  _debounceDelay(debounceDelay),
  _lastPhysicalState(HIGH),
  _buttonIsCurrentlyPressed(false),
  _wasJustPressedFlag(false),
  _lastDebounceTime(0)
{}

void Button::begin() {
  pinMode(_pin, INPUT_PULLUP);
}

void Button::update() {
  bool reading = digitalRead(_pin);
  unsigned long currentTime = millis();

  // If the switch changed, due to noise or pressing:
  if (reading != _lastPhysicalState) {
    _lastDebounceTime = currentTime;
    _lastPhysicalState = reading;
  }

  if ((currentTime - _lastDebounceTime) > _debounceDelay) {
    // If reading is LOW, button is physically held down
    if (reading == LOW && !_buttonIsCurrentlyPressed) {
      _buttonIsCurrentlyPressed = true;
    }
    // If reading is HIGH and it WAS pressed, register a click
    else if (reading == HIGH && _buttonIsCurrentlyPressed) {
      _buttonIsCurrentlyPressed = false;
      _wasJustPressedFlag = true; // Event triggers on Release
    }
  }
}

bool Button::wasJustPressed() {
  if (_wasJustPressedFlag) {
    _wasJustPressedFlag = false; // Reset flag after reading
    return true;
  }
  return false;
}

bool Button::isBeingHeld() {
  return _buttonIsCurrentlyPressed;
}