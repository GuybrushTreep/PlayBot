/**
 * Robot Controller - Main Program
 * 
 * This program controls a robot with various sensors and capabilities including:
 * - Motor control with encoders and PID
 * - Distance sensing (ToF sensors)
 * - Edge detection (IR sensors)
 * - Battery monitoring
 * - LED status indication
 * - Light sensing
 * - USB communication with host device
 * - SD card animation playback
 * - Servo control
 * 
 * Hardware requirements:
 * - Teensy board
 * - DRV8835 motor driver
 * - WS2812 LED
 * - ToF sensors with PCA9540BD multiplexer
 * - IR sensors
 * - MAX17048 battery gauge
 * - Servo motor
 * - SD card 
 */

// ================= Library Includes =================
#include "Config.h"
#include "HardwareConfig.h"
#include "Debug.h"
#include "StorageManager.h"
#include "DistanceTracker.h"
#include "LEDController.h"
#include "BatteryManager.h"
#include "AnimationManager.h"
#include "SensorManager.h"
#include "MotorController.h"
#include "CommunicationManager.h"
#include <string>
#include <PID_v1.h>
#include <SD.h>
#include <vector>
#include <Smoothed.h>

// ================= Global Objects =================
LEDController ledController(ws2812fx);
AnimationManager animationManager(headServo, motors, myEnc, myEnc2);
BatteryManager batteryManager(userial, ledController, animationManager);
DistanceTracker distanceTracker(myEnc, myEnc2);
String rightWheel, leftWheel;
SensorManager sensorManager(userial, animationManager, motors, ws2812fx, mux, 
                          myEnc, myEnc2, batteryManager, distanceTracker);
MotorController motorController(
    motors, 
    myEnc,      // Left encoder
    myEnc2,     // Right encoder
    myPID2,     // Left PID
    myPID,      // Right PID
    Setpoint,   // Right setpoint
    Setpoint2,  // Left setpoint
    Input,      // Right input
    Input2,     // Left input
    Output,     // Right output
    Output2     // Left output
);
StorageManager storageManager;
CommunicationManager communicationManager(
    myusb,
    userial,
    animationManager,
    batteryManager,
    sensorManager,
    motorController
);

// ================= Global Variables =================
bool MOTION_ENABLED = true;  // Initialize as enabled
unsigned long currentMillis = 0;

// ================= Main Setup Function =================
void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    delay(1000);
    DEBUG_PRINT(DEBUG_INFO, "Setup started");
    //Initialize classes
    storageManager.initialize();
    sensorManager.initialize();
    ledController.initialize();
    motorController.initialize();
    batteryManager.initialize();
    communicationManager.initialize();
    distanceTracker.initialize();
    printSetupErrorSummary();

    DEBUG_PRINT(DEBUG_INFO, "Setup completed");

    // Set LED status based on setup success
    if (setupErrors.empty()) {
        ledController.setStatusSuccess();  // Green for success
    } else {
        ledController.setStatusError();    // Red for error
    }

    sendLogs();
}

// ================= Main Loop Function =================
void loop() {
    ledController.update();

    // Update system states
    motorController.updateTimers();
    motorController.updateEncoders();
    motorController.computePID();

    animationManager.getWheelCommands(rightWheel, leftWheel);
    motorController.updateSetpoints(rightWheel, leftWheel, animationManager.isAnimationPlaying());
    motorController.controlMotors(animationManager.isAnimationPlaying(), MOTION_ENABLED);

    // Handle animation if active
    animationManager.update();

    // Process USB communication and sensor checks
    communicationManager.readFromUSBHostSerialAndWriteToSerial();
    communicationManager.handleBaudRateChange();
    
    sensorManager.detectTableEdgeIR();
    sensorManager.checkLightSensor();
    sensorManager.checkFrontCollision();

    distanceTracker.update();
    distanceTracker.checkAndLog(animationManager.isAnimationPlaying());

    batteryManager.detectBatteryCharging();
    sendLogs();
    DEBUG_PRINT(DEBUG_VERBOSE, "Loop iteration completed");
}



