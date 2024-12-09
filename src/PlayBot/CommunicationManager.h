// CommunicationManager.h
#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include "Config.h"
#include "Debug.h"
#include "AnimationManager.h"
#include "BatteryManager.h"
#include "SensorManager.h"
#include "MotorController.h"

// Manages bidirectional communication between Teensy and Playdate
// Playdate -> Teensy commands:
// - "a/filepath" : Start animation from SD
// - "b" : Request battery status
// - "d" : Request sensor data
// - "v" : Verify connection
// - "t/turns/direction" : Turn robot
// - "x" : Stop animation
//
// Teensy -> Playdate messages:
// - "msg b/percent/voltage/charging" : Battery status
// - "msg p/0|1" : Power state change
// - "msg s/" : Connection confirmation
// - "msg d/..." : Sensor data packet
// - "msg r/1" : Rotation completed
// - "msg e/1" : Edge detected
// - "msg w/1" : Collision detected
// - "msg l/0|1" : Light level change
class CommunicationManager {
private:
    // Hardware and subsystem references
    USBHost& myusb;                       // USB host interface
    USBSerial_BigBuffer& userial;         // Serial communication
    AnimationManager& animationManager;    // Animation control
    BatteryManager& batteryManager;       // Battery monitoring
    SensorManager& sensorManager;         // Sensor readings
    MotorController& motorController;     // Motor control
    
    // Communication settings
    uint32_t baud;                        // Current baud rate
    uint32_t format;                      // Serial format
    char buffer[512];                     // Message buffer

public:
    // Initialize with references to all required subsystems
    CommunicationManager(
        USBHost& usb,
        USBSerial_BigBuffer& serial,
        AnimationManager& anim,
        BatteryManager& battery,
        SensorManager& sensor,
        MotorController& motor
    ) : myusb(usb),
        userial(serial),
        animationManager(anim),
        batteryManager(battery),
        sensorManager(sensor),
        motorController(motor),
        baud(USBBAUD),
        format(USBHOST_SERIAL_8N1)
    {
    }

    // Initialize USB communication
    void initialize() {
        DEBUG_PRINT(DEBUG_INFO, "Initializing USB");
        myusb.begin();
        userial.begin(USBBAUD);
        DEBUG_PRINT(DEBUG_INFO, "USB initialized");
        delay(500);  // Allow USB to stabilize
    }

    // Monitor for baud rate changes from Playdate
    // Needed to maintain stable communication
    void handleBaudRateChange() {
        uint32_t cur_usb_baud = Serial.baud();
        if (cur_usb_baud && (cur_usb_baud != baud)) {
            baud = cur_usb_baud;
            DEBUG_PRINT(DEBUG_INFO, "Baud rate changed: " + String(baud));
            // Special case for 57600 baud - use 58824 instead
            userial.begin(baud == 57600 ? 58824 : baud);
        }
    }

    // Process incoming messages from Playdate
    // This is the main communication handler
    void readFromUSBHostSerialAndWriteToSerial() {
        uint16_t rd = userial.available();
        if (rd > 0) {
            // Limit message size to prevent buffer overflow
            if (rd > 80) rd = 80;
            userial.readBytes((char*)buffer, rd);

            // Ignore 'c/' commands (handled elsewhere)
            if (buffer[0] != 'c' || buffer[1] != '/') {
                char messageType = buffer[0];
                
                switch(messageType) {
                    case 'a':  // Start animation
                        handleAnimationMessage();
                        break;
                    case 'b':  // Battery status request
                        batteryManager.getBatteryLevel();
                        break;
                    case 'd':  // Sensor data request
                        sensorManager.sendSensorData();
                        break;
                    case 'v':  // Connection verification
                        userial.println("msg s/");
                        break;
                    case 't':  // Turn robot command
                        handleCrankTurns();
                        break;
                    case 'x':  // Stop animation
                        animationManager.stopAnimation();
                        motors.setM1Speed(0);
                        motors.setM2Speed(0);
                        DEBUG_PRINT(DEBUG_INFO, "Animation stopped by options menu");
                        break;
                }
            }
        }
    }

private:
    // Handle animation start command from Playdate
    // Format: "a/filepath"
    void handleAnimationMessage() {
        DEBUG_PRINT(DEBUG_INFO, "Handling animation message");
        char* animPath = strchr((char*)buffer, '/');
        if (animPath != NULL) {
            animPath++; // Skip the '/'
            // Clean non-printable characters
            for (int i = 0; animPath[i]; i++) {
                if (!isprint(animPath[i])) {
                    animPath[i] = '\0';
                }
            }
            animationManager.startAnimation(animPath);
        }
        DEBUG_PRINT(DEBUG_INFO, "Animation prepared: " + String(animPath));
    }

    // Handle turn command from Playdate
    // Format: "t/number_of_turns/direction"
    // Direction: 1 = clockwise, -1 = counterclockwise
    void handleCrankTurns() {
        char* turnData = strchr((char*)buffer, '/') + 1;
        int turns = atoi(turnData);
        int direction = atoi(strchr(turnData, '/') + 1);
        if(turns > 0) {
            motorController.rotateRobot(turns, direction);
        }
    }
};

#endif // COMMUNICATION_MANAGER_H