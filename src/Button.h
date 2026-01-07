/**
 * @file Button.h
 * @author Ebrahim Siami
 * @brief Button Class Interface
 * @version 2.1.3
 * @date 2025-02-06
 *
 * Description:
 * Defines the structure for handling physical buttons with 
 * internal pull-up resistors (Active-Low configuration) 
 * and non-blocking software debouncing.
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
public:
    /**
     * @brief Construct a new Button object.
     * 
     * @param pin The STM32 GPIO pin connected to the button.
     * @param debounceDelay Time in ms to wait for signal stabilization (default: 50ms).
     */
    Button(int pin, long debounceDelay = 50);

    /**
     * @brief Configures the pin as INPUT_PULLUP.
     * Must be called in setup() before using the button.
     */
    void begin();

    /**
     * @brief Reads the physical button state and updates internal logic.
     * CRITICAL: Must be called continuously in the main loop() (polling).
     */
    void update();

    /**
     * @brief Checks if a "Click" event occurred (Press followed by Release).
     * This flag is cleared after reading (consume-on-read).
     * 
     * @return true if the button was just released.
     */
    bool wasJustPressed();

    /**
     * @brief Checks the real-time debounced state.
     * 
     * @return true while the button is physically held down.
     */
    bool isBeingHeld();

private:
    int _pin;                       // Hardware pin number
    long _debounceDelay;            // Noise filter duration in ms
    
    bool _lastPhysicalState;        // Immediate reading from the previous loop
    bool _buttonIsCurrentlyPressed; // Stable, debounced state (True = Held)
    bool _wasJustPressedFlag;       // Event flag for single click detection
    
    unsigned long _lastDebounceTime; // Timestamp of the last state change
};

#endif // BUTTON_H