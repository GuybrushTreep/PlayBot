// DistanceTracker.h
#ifndef DISTANCE_TRACKER_H
#define DISTANCE_TRACKER_H

#include "Config.h"
#include "Debug.h"
#include <SD.h>

// Class to track total distance traveled by the robot
// Maintains record of distance traveled by both wheels
// and stores/loads the data from persistent storage (SD card)
class DistanceTracker {
private:
    // Internal structure to hold distance data
    struct DistanceData {
        float leftDistance;      // Total distance traveled by left wheel (mm)
        float rightDistance;     // Total distance traveled by right wheel (mm)
        float averageDistance;   // Average of both wheels (mm)
        long lastLeftPosition;   // Last encoder position for left wheel
        long lastRightPosition;  // Last encoder position for right wheel
        unsigned long lastUpdateTime;  // Timestamp of last update
    } totalDistance;

    const char* LOG_FILENAME = "distance.txt";  // File for persistent storage
    unsigned long lastDistanceLogTime;          // Timestamp for periodic logging
    Encoder& encoderLeft;                       // Reference to left wheel encoder
    Encoder& encoderRight;                      // Reference to right wheel encoder

public:
    // Constructor - initializes tracking with encoders
    DistanceTracker(Encoder& left, Encoder& right) 
        : encoderLeft(left)
        , encoderRight(right)
        , lastDistanceLogTime(0) {
        totalDistance = {0, 0, 0, 0, 0, 0};
    }

    // Initialize distance tracking and load previous data
    // Called during system startup to restore previous distance data
    void initialize() {
        DEBUG_PRINT(DEBUG_INFO, "Starting distance initialization");
        loadFromFile();  // Load previous distance from SD card
        // Reset encoder positions for new tracking session
        totalDistance.lastLeftPosition = encoderLeft.read();
        totalDistance.lastRightPosition = encoderRight.read();
        totalDistance.lastUpdateTime = millis();
    }

    // Update distance calculations based on encoder readings
    // Should be called regularly in the main loop
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - totalDistance.lastUpdateTime >= DISTANCE_UPDATE_INTERVAL) {
            // Get current encoder positions
            long currentLeftPos = encoderLeft.read();
            long currentRightPos = encoderRight.read();

            // Calculate incremental distances since last update
            float leftIncrement = abs(currentLeftPos - totalDistance.lastLeftPosition) * MM_PER_TICK;
            float rightIncrement = abs(currentRightPos - totalDistance.lastRightPosition) * MM_PER_TICK;

            // Update total distances
            totalDistance.leftDistance += leftIncrement;
            totalDistance.rightDistance += rightIncrement;
            totalDistance.averageDistance = (totalDistance.leftDistance + totalDistance.rightDistance) / 2.0f;

            // Store current positions for next update
            totalDistance.lastLeftPosition = currentLeftPos;
            totalDistance.lastRightPosition = currentRightPos;
            totalDistance.lastUpdateTime = currentTime;
        }
    }

    // Check if it's time to log distance and do so if needed
    void checkAndLog(bool isAnimating) {
        unsigned long currentTime = millis();
        if (currentTime - lastDistanceLogTime >= DISTANCE_LOG_INTERVAL && !isAnimating) {
            saveToFile();
            lastDistanceLogTime = currentTime;
            DEBUG_PRINT(DEBUG_INFO, "Distance auto-logged");
        }
    }

    // Get total distance traveled in meters
    float getDistanceMeters() const {
        return totalDistance.averageDistance / 1000.0f;  // Convert mm to m
    }

    // Save current distance to SD card
    // Handles the persistent storage of distance data
    void saveToFile() {
        File distanceLog = SD.open(LOG_FILENAME, FILE_WRITE);
        if (distanceLog) {
            distanceLog.seek(0);
            distanceLog.truncate();
            char distanceStr[10];
            dtostrf(getDistanceMeters(), 1, 2, distanceStr);
            distanceLog.print(distanceStr);
            distanceLog.close();
            DEBUG_PRINT(DEBUG_INFO, "Distance logged: " + String(getDistanceMeters()) + "m");
        } else {
            DEBUG_PRINT(DEBUG_WARNING, "Failed to open distance file");
        }
    }

private:
    // Load previously saved distance from SD card
    // Called during initialization to restore the last saved state
    void loadFromFile() {
        if (SD.exists(LOG_FILENAME)) {
            File distanceLog = SD.open(LOG_FILENAME, FILE_READ);
            if (distanceLog) {
                String distanceStr = distanceLog.readStringUntil('\n');
                distanceStr.trim();
                float savedDistance = distanceStr.toFloat();
                
                // Convert loaded meters to millimeters and set all distances
                totalDistance.averageDistance = savedDistance * 1000.0f;
                totalDistance.leftDistance = totalDistance.averageDistance;
                totalDistance.rightDistance = totalDistance.averageDistance;
                
                distanceLog.close();
                DEBUG_PRINT(DEBUG_INFO, "Distance loaded: " + String(savedDistance) + "m");
            }
        }
    }
};

#endif // DISTANCE_TRACKER_H