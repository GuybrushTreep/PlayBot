// Debug.h
#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include <vector>
#include <string>
#include <CircularBuffer.hpp>
#include "HardwareConfig.h"

// Debug system for robot firmware
// Handles logging, error tracking, and setup verification
// Integrates with Playdate communication through 'msg s/' messages

// ================= Debug Levels =================
// Hierarchical debug levels from none to verbose
#define DEBUG_NONE 0      // No debug output
#define DEBUG_INFO 1      // Important state changes and events
#define DEBUG_WARNING 2   // Issues that need attention
#define DEBUG_VERBOSE 3   // Detailed operational data
#define CURRENT_DEBUG_LEVEL DEBUG_INFO  // Set active debug level

// ================= Buffer Configuration =================
// Circular buffer to prevent memory issues with long-running logs
#define LOG_BUFFER_SIZE 50            // Maximum stored log entries
#define LOG_SEND_INTERVAL 1000        // Milliseconds between log transmissions

// ================= Global Variables =================
// Circular buffer for runtime logs
static CircularBuffer<String, LOG_BUFFER_SIZE> logBuffer;

// Timestamp for managing log transmission
static unsigned long lastLogSendTime = 0;

// Collection of errors encountered during setup
// These trigger 'msg s/' failure messages to Playdate
static std::vector<std::string> setupErrors;

// ================= Debug Macro =================
// Main logging macro - automatically includes level and formats message
// Usage: DEBUG_PRINT(DEBUG_INFO, "Battery voltage: " + String(voltage));
#define DEBUG_PRINT(level, message) \
    if (level <= CURRENT_DEBUG_LEVEL) { \
        String logMessage = String("DEBUG [") + String(#level) + String("]: ") + String(message); \
        logBuffer.push(logMessage); \
    }

// ================= Function Implementations =================

// Transmits accumulated logs at regular intervals
// Called in main loop to maintain communication flow
inline void sendLogs() {
    unsigned long currentTime = millis();
    if (currentTime - lastLogSendTime >= LOG_SEND_INTERVAL) {
        while (!logBuffer.isEmpty()) {
            Serial.println(logBuffer.shift());
        }
        lastLogSendTime = currentTime;
    }
}

// Records setup phase errors
// These errors affect the success/failure message sent to Playdate
inline void recordSetupError(const String& errorMessage) {
    setupErrors.push_back(errorMessage.c_str());
    DEBUG_PRINT(DEBUG_WARNING, "Setup Error: " + errorMessage);
}

// Prints setup error summary and notifies Playdate
// Sends "msg s/1" for success or logs errors for failure
inline void printSetupErrorSummary() {
    if (setupErrors.empty()) {
        Serial.println("Setup completed successfully with no errors.");
        DEBUG_PRINT(DEBUG_INFO, "Setup completed successfully with no errors.");
        userial.println("msg s/1");  // Signal success to Playdate
    } else {
        Serial.println("Setup completed with the following errors:");
        for (const auto& error : setupErrors) {
            Serial.println("- " + String(error.c_str()));
        }
        DEBUG_PRINT(DEBUG_WARNING, "Setup completed with errors:");
        for (const auto& error : setupErrors) {
            DEBUG_PRINT(DEBUG_WARNING, "- " + String(error.c_str()));
        }
        // Note: No explicit failure message sent to Playdate
        // Absence of "msg s/1" indicates setup failure
    }
}

#endif // DEBUG_H