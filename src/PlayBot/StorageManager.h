// StorageManager.h
#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "Config.h"
#include "Debug.h"
#include <SD.h>

// Manages SD card storage for robot animations and data
// The SD card stores:
// - Animation files (loaded when Playdate sends "a/filepath")
// - Distance logs (updated periodically by DistanceTracker)
// Initialization failure will trigger error message to Playdate
class StorageManager {
private:
    bool isInitialized;   // Tracks if SD card is ready for use

public:
    // Constructor - Sets initial state
    StorageManager() : isInitialized(false) {}

    // Initialize SD card system
    // Called during setup phase
    // Returns true if card is accessible and ready for use
    bool initialize() {
        DEBUG_PRINT(DEBUG_INFO, "Initializing SD card");
        
        // Try to initialize SD card on the hardware pin
        if (!SD.begin(SD_CS_PIN)) {
            // Card init failed - record error
            // This will prevent "msg s/1" success message to Playdate
            recordSetupError("SD card initialization failed");
            DEBUG_PRINT(DEBUG_WARNING, "SD card initialization failed!");
            isInitialized = false;
            return false;
        } else {
            // Card successfully initialized
            DEBUG_PRINT(DEBUG_INFO, "SD card initialized");
            isInitialized = true;
            return true;
        }
    }

    // Check if SD card is ready for use
    // Used by AnimationManager before file operations
    bool isReady() const { return isInitialized; }


};

#endif // STORAGE_MANAGER_H