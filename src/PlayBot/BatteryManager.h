// BatteryManager.h
#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include "Config.h"
#include "Debug.h"
#include "HardwareConfig.h"
#include "LEDController.h"
#include "AnimationManager.h"
#include <Smoothed.h>

// Manages battery monitoring and charging state
// Communicates with Playdate using following messages:
// - "msg b/percent/voltage/charging" : Battery status update
// - "msg p/1" : Charging started
// - "msg p/0" : Charging stopped
class BatteryManager {
private:
    // Hardware state tracking
    bool max1704x_initialized;     // MAX17048 battery gauge initialization status
    bool chargingState;           // Current charging status (true = charging)
    unsigned long lastBatteryCheckTime; 
    bool firstReadingTaken;       // Tracks if we have initial reading
    int consecutiveReadings;      // Count of consistent readings
    
    // Hardware references
    USBSerial_BigBuffer& userial;  // Serial communication for Playdate messages
    LEDController& ledController;   // LED status indicator
    AnimationManager& animationManager; // Used to avoid battery updates during animations
    
    // Smoothing filters for stable readings
    Smoothed<float> smoothedVoltage;
    Smoothed<float> smoothedChargeRate;

    // Logs battery errors to debug system
    void recordBatteryError(const String& errorMessage) {
        setupErrors.push_back(errorMessage.c_str());
        DEBUG_PRINT(DEBUG_WARNING, "Battery Manager Error: " + errorMessage);
    }

    // Sets up data smoothing for voltage readings
    void initializeBatteryDetection() {
        smoothedVoltage.begin(SMOOTHED_AVERAGE, 2);
        smoothedChargeRate.begin(SMOOTHED_AVERAGE, 2);
        DEBUG_PRINT(DEBUG_INFO, "Battery detection initialized");
    }

public:
    // Constructor initializes all dependencies
    BatteryManager(USBSerial_BigBuffer& serial, LEDController& led, AnimationManager& anim) 
        : max1704x_initialized(false)
        , chargingState(false)
        , lastBatteryCheckTime(0)
        , firstReadingTaken(false)
        , consecutiveReadings(0)
        , userial(serial)
        , ledController(led)
        , animationManager(anim) {
    }

    // Initialize battery monitoring system
    // Returns true if MAX17048 gauge initialized successfully
    bool initialize() {
        DEBUG_PRINT(DEBUG_INFO, "Initializing battery systems");
        delay(500);  // Allow gauge to stabilize
        
        initializeBatteryDetection();
        
        if (maxlipo.begin()) {
            DEBUG_PRINT(DEBUG_INFO, "MAX1704X initialized successfully");
            max1704x_initialized = true;
            return true;
        } else {
            recordBatteryError("Battery gauge (MAX1704X) initialization failed");
            max1704x_initialized = false;
            return false;
        }
    }

    // Check for USB power connection changes
    // Sends "msg p/1" or "msg p/0" to Playdate on state change
    void detectBatteryCharging() {
        unsigned long currentTime = millis();
        if (currentTime - lastBatteryCheckTime >= BATTERY_CHECK_INTERVAL) {
            // Read USB detection pin voltage
            int adcValue = analogRead(USB_DETECT_PIN);
            float pinVoltage = (adcValue / 1023.0) * 3.3;
            
            bool newChargingState = (pinVoltage > 1.5); // USB present if > 1.5V
            
            if (newChargingState != chargingState) {
                chargingState = newChargingState;
                
                // Disable robot motion while charging
                MOTION_ENABLED = !chargingState;
                
                if (chargingState) {
                    motors.setM1Speed(0);
                    motors.setM2Speed(0);
                    ledController.setStatusCharging();
                } else {
                    ledController.setStatusIdle();
                }
                
                userial.println(chargingState ? "msg p/1" : "msg p/0");
                DEBUG_PRINT(DEBUG_INFO, "Charging state changed: " + 
                          String(chargingState ? "Connected" : "Disconnected") +
                          " Pin Voltage: " + String(pinVoltage) +
                          "V Motion: " + String(MOTION_ENABLED ? "Enabled" : "Disabled"));
            }
            
            lastBatteryCheckTime = currentTime;
        }
    }

    // Format and send battery status to Playdate
    // Returns formatted message string
    // Format: "msg b/percent/voltage/charging"
    char* getBatteryLevel() {
        static char message[40];  // Static buffer for message
        
        // Only send update if no animation is playing
        if (!animationManager.isAnimationPlaying()) {
            if (max1704x_initialized) {
                float voltage = maxlipo.cellVoltage();
                float percent = maxlipo.cellPercent();
                 int alertLevel = 0;
            if (percent <= BATTERY_CRITICAL_THRESHOLD) {
                alertLevel = 2;

            }   else if (percent <= BATTERY_LOW_THRESHOLD) {
                alertLevel = 1;
            }


                // Format floating point numbers
                char percentBuffer[10];
                char voltageBuffer[10];
                dtostrf(percent, 6, 2, percentBuffer);
                dtostrf(voltage, 6, 2, voltageBuffer);

                 snprintf(message, sizeof(message), "msg b/%s/%s/%d/%d", 
                    percentBuffer, voltageBuffer, chargingState ? 1 : 0, alertLevel);
            DEBUG_PRINT(DEBUG_INFO, "Battery: " + String(voltage) + "V, " + 
                       String(percent) + "%, Charging: " + String(chargingState) + 
                       ", Alert: " + String(alertLevel));
            } else {
                  snprintf(message, sizeof(message), "msg b/NA/NA/NA/0");
            DEBUG_PRINT(DEBUG_WARNING, "Battery monitoring not available");
            message[0] = '\0';  // Empty message if animation playing
            }

            userial.println(message);
        } else {
            message[0] = '\0';  // Empty message if animation playing
        }
        
        return message;
    }

    // Getters for battery state
    bool isCharging() const { return chargingState; }
    float getVoltage() const { return max1704x_initialized ? maxlipo.cellVoltage() : 0.0; }
};

#endif // BATTERY_MANAGER_H