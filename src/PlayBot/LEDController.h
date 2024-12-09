// LEDController.h
#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <WS2812FX.h>
#include "Config.h"

// Controls the WS2812 RGB LED that indicates robot status
// The LED states correspond to messages exchanged between Teensy and Playdate:
// - Charging (Blue breath): Triggered by "msg p/1" message
// - Error (Red fade): Setup or runtime errors
// - Success (Green fade): Successful initialization ("msg s/1")
// - Idle (Pink fade): Default state, robot ready for commands
class LEDController {
private:
    WS2812FX& strip;           // Reference to WS2812FX LED strip object
    uint8_t brightness;         // Current LED brightness (5-255)
    bool isInitialized;         // Tracks if LED has been properly initialized

public:
    // Constructor takes reference to existing WS2812FX object to avoid duplication
    explicit LEDController(WS2812FX& ledStrip) 
        : strip(ledStrip)
        , brightness(5)        // Start with low brightness to avoid blinding
        , isInitialized(false) {
    }
    
    // Initialize LED hardware and set default state
    // Returns true if initialization successful
    bool initialize() {
        strip.init();
        strip.setBrightness(brightness);
        strip.setColor(PINK);  // Default color - matches Playdate's idle state
        // Set up default fade animation
        strip.setSegment(0, 0, LED_COUNT-1, FX_MODE_FADE, COLORS(BLACK, PINK), 20, false);
        strip.start();
        isInitialized = true;
        DEBUG_PRINT(DEBUG_INFO, "LED Controller initialized");
        return true;
    }

    // Must be called in main loop to update LED animations
    void update() {
        if (isInitialized) {
            strip.service();
        }
    }

    // Set LED to green fade - indicates successful setup
    // Called after "msg s/1" is sent to Playdate
    void setStatusSuccess() {
        if (!isInitialized) return;
        strip.setMode(FX_MODE_FADE);
        strip.setColor(GREEN);
        strip.setSegment(0, 0, LED_COUNT-1, FX_MODE_FADE, COLORS(BLACK, GREEN), 20, false);
        strip.start();
        DEBUG_PRINT(DEBUG_INFO, "LED status: Success");
    }

    // Set LED to red fade - indicates error state
    // Triggered by setup errors or runtime issues
    void setStatusError() {
        if (!isInitialized) return;
        strip.setMode(FX_MODE_FADE);
        strip.setColor(RED);
        strip.setSegment(0, 0, LED_COUNT-1, FX_MODE_FADE, COLORS(BLACK, RED), 20, false);
        strip.start();
        DEBUG_PRINT(DEBUG_INFO, "LED status: Error");
    }

    // Set LED to blue breathing effect - indicates charging
    // Triggered when "msg p/1" is sent to Playdate
    void setStatusCharging() {
        if (!isInitialized) return;
        strip.setMode(FX_MODE_BREATH);
        strip.setColor(BLUE);
        strip.setSegment(0, 0, LED_COUNT-1, FX_MODE_BREATH, COLORS(BLACK, BLUE), 1000, false);
        strip.start();
        DEBUG_PRINT(DEBUG_INFO, "LED status: Charging");
    }

    // Set LED to pink fade - indicates idle/ready state
    // Default state when robot is operational
    void setStatusIdle() {
        if (!isInitialized) return;
        strip.setMode(FX_MODE_FADE);
        strip.setColor(PINK);
        strip.setSegment(0, 0, LED_COUNT-1, FX_MODE_FADE, COLORS(BLACK, PINK), 20, false);
        strip.start();
        DEBUG_PRINT(DEBUG_INFO, "LED status: Idle");
    }

    // Adjust LED brightness based on ambient light
    // Syncs with Playdate's light sensor handling ("msg l/0" and "msg l/1")
    void adjustBrightness(bool isDark) {
        setBrightness(isDark ? 50 : 10);
    }

    // Set specific brightness level (5-255)
    void setBrightness(uint8_t level) {
        if (!isInitialized) return;
        brightness = level;
        strip.setBrightness(brightness);
        DEBUG_PRINT(DEBUG_VERBOSE, "LED brightness set to: " + String(brightness));
    }
    
    // Check if LED controller is ready
    bool isReady() const { return isInitialized; }
};

#endif // LED_CONTROLLER_H