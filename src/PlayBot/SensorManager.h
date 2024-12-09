// SensorManager.h
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "Config.h"
#include "Debug.h"
#include "BatteryManager.h"
#include "DistanceTracker.h"
#include <Wire.h>
#include <Smoothed.h>

// Manages all robot sensors and communicates with Playdate via messages:
// - "msg d/..." : Complete sensor data packet
// - "msg e/1" : Edge detected
// - "msg w/1" : Collision detected
// - "msg l/0|1" : Dark/Light state change
class SensorManager {
private:
    // ToF sensor variables with exponential smoothing 
    // to handle unreliable sensors
    Smoothed<float> TOFsensor1;        // Front sensor smoothing
    Smoothed<float> TOFsensor2;        // Back sensor smoothing
    float TOFsensorFront = 0;          // Cached front distance
    float TOFsensorBack = 0;           // Cached back distance
    unsigned char i2c_rx_buf[16];      // I2C reading buffer
    unsigned short lenth_val = 0;      // Current distance value

    // Light sensor state tracking
    unsigned long lastLightCheckTime = 0;
    bool isInDarkness = false;
    bool previousDarknessState = false;

    // Collision detection with enhanced validation
    unsigned long lastCollisionCheckTime = 0;
    bool collisionMessageSent = false;

    // IR edge detection with adjustable thresholds
    int IR_THRESHOLD_LEFT = 15;         // Left sensor threshold
    int IR_THRESHOLD_RIGHT = 15;        // Right sensor threshold
    unsigned long lastIRCheckTime = 0;
    bool isEdgeDetectedIR = false;
    Smoothed<float> smoothedIRLeft;     // IR value smoothing
    Smoothed<float> smoothedIRRight;    // IR value smoothing

    // Hardware interface references
    USBSerial_BigBuffer& userial;       // Serial communication
    AnimationManager& animationManager;  // Animation control
    DRV8835MotorShield& motors;         // Motor control
    WS2812FX& ws2812fx;                 // LED control
    PCA9540BD& mux;                     // ToF multiplexer
    Encoder& encoderLeft;               // Left wheel encoder
    Encoder& encoderRight;              // Right wheel encoder
    BatteryManager& batteryManager;     // Battery monitoring
    DistanceTracker& distanceTracker;   // Distance tracking

    // Read raw data from ToF sensor via I2C
    void SensorRead(unsigned char addr, unsigned char* datbuf, unsigned char cnt) {
        Wire.beginTransmission(82);
        Wire.write(byte(addr));
        Wire.endTransmission();
        Wire.requestFrom((uint8_t)82, (uint8_t)cnt);
        if (cnt <= Wire.available()) {
            *datbuf++ = Wire.read();
            *datbuf++ = Wire.read();
        }
    }

    // Convert raw ToF data to distance value
    int ReadDistance() {
        SensorRead(0x00, i2c_rx_buf, 2);
        lenth_val = i2c_rx_buf[0];
        lenth_val = lenth_val << 8;
        lenth_val |= i2c_rx_buf[1];
        return lenth_val;
    }

public:
    // Initialize manager with hardware references
    SensorManager(USBSerial_BigBuffer& serial, AnimationManager& anim, 
                 DRV8835MotorShield& mot, WS2812FX& led, PCA9540BD& multiplexer,
                 Encoder& encLeft, Encoder& encRight,
                 BatteryManager& battery, DistanceTracker& distance)
        : userial(serial)
        , animationManager(anim)
        , motors(mot)
        , ws2812fx(led)
        , mux(multiplexer)
        , encoderLeft(encLeft)
        , encoderRight(encRight)
        , batteryManager(battery)
        , distanceTracker(distance) {
        // Configure smoothing filters
        smoothedIRLeft.begin(SMOOTHED_EXPONENTIAL, 1000);
        smoothedIRRight.begin(SMOOTHED_EXPONENTIAL, 1000);
        TOFsensor1.begin(SMOOTHED_EXPONENTIAL, 2);    // Increased smoothing for unreliable ToF
        TOFsensor2.begin(SMOOTHED_EXPONENTIAL, 2);
    }

    // Initialize sensor hardware
    void initialize() {
        pinMode(IR_SENSOR_RIGHT_PIN, INPUT_DISABLE);
        pinMode(IR_SENSOR_LEFT_PIN, INPUT_DISABLE);
        DEBUG_PRINT(DEBUG_INFO, "IR sensors initialized");

        Wire.begin();
        DEBUG_PRINT(DEBUG_INFO, "ToF sensors initialized");
    }

    // Read distance from specific ToF sensor
    // Includes caching for reliability
    float readTofSensor(int channel) {
        mux.selectChannel(channel);
        float rawDistance = ReadDistance();
        
        if (rawDistance > 0) {
            if (channel == FRONT_SENSOR) {
                TOFsensorFront = rawDistance;
                return TOFsensorFront;
            } else {
                TOFsensorBack = rawDistance;
                return TOFsensorBack;
            }
        }
        
        return (channel == FRONT_SENSOR) ? TOFsensorFront : TOFsensorBack;
    }

    // Read averaged IR sensor values
    int readIRSensor(int sensorPin) {
        long sum = 0;
        const int numReadings = 10;
        
        for (int i = 0; i < numReadings; i++) {
            sum += analogRead(sensorPin);
        }
        return sum / numReadings;
    }

    // Monitor ambient light changes
    void checkLightSensor() {
        unsigned long currentTime = millis();
        if (currentTime - lastLightCheckTime >= LIGHT_CHECK_INTERVAL) {
            int lightValue = analogRead(LIGHT_SENSOR_PIN);
            bool currentDarknessState = (lightValue < DARKNESS_THRESHOLD);
            
            if (currentDarknessState != previousDarknessState) {
                isInDarkness = currentDarknessState;
                userial.println(isInDarkness ? "msg l/0" : "msg l/1");
                previousDarknessState = isInDarkness;
                DEBUG_PRINT(DEBUG_VERBOSE, "Light state changed. Is in darkness: " + String(isInDarkness));
            }
            
            lastLightCheckTime = currentTime;
            ws2812fx.setBrightness(isInDarkness ? 50 : 10);
        }
    }

    // Detect table edges using IR sensors
    void detectTableEdgeIR() {
        unsigned long currentTime = millis();
        if (currentTime - lastIRCheckTime >= IR_CHECK_INTERVAL) {
            float sensorLeftValue = readIRSensor(IR_SENSOR_LEFT_PIN);
            float sensorRightValue = readIRSensor(IR_SENSOR_RIGHT_PIN);
            
            bool currentEdgeDetected = (sensorLeftValue >= IR_THRESHOLD_LEFT || 
                                      sensorRightValue >= IR_THRESHOLD_RIGHT);
            
            if (currentEdgeDetected != isEdgeDetectedIR) {
                isEdgeDetectedIR = currentEdgeDetected;
                
                if (isEdgeDetectedIR) {
                    animationManager.stopAnimation();
                    motors.setM1Speed(0);
                    motors.setM2Speed(0);
                    userial.println("msg e/1");
                    DEBUG_PRINT(DEBUG_INFO, "Edge detected by IR sensors. Left: " + 
                              String(sensorLeftValue) + ", Right: " + String(sensorRightValue));
                }
            }
            
            lastIRCheckTime = currentTime;
        }
    }

    // Check for front collisions with enhanced ToF validation
    // Uses multiple readings and threshold to handle unreliable sensors --> slow :(, V2 will use an array of IR sensors 
    void checkFrontCollision() {
        unsigned long currentTime = millis();
        if (currentTime - lastCollisionCheckTime >= COLLISION_CHECK_INTERVAL) {
            float frontDistance = readTofSensor(FRONT_SENSOR);
            
            // Strict range validation to filter void detections
            if (frontDistance > 0 && frontDistance < 1800) {
                TOFsensor2.add(frontDistance);
                float smoothedFrontDistance = TOFsensor2.get();
                
                static uint8_t collisionCount = 0;
                static uint8_t validationThreshold = 30;  // Increased for reliability
                
                if (smoothedFrontDistance < FRONT_COLLISION_THRESHOLD) {
                    collisionCount++;
                    if (collisionCount >= validationThreshold && !collisionMessageSent) {
                        userial.println("msg w/1");
                        collisionMessageSent = true;
                        DEBUG_PRINT(DEBUG_INFO, "Front collision validated. Distance: " + 
                                  String(smoothedFrontDistance) + "mm, Raw: " + 
                                  String(frontDistance) + "mm");
                    }
                } else {
                    collisionCount = 0;
                    collisionMessageSent = false;
                }
            }
            
            lastCollisionCheckTime = currentTime;
        }
    }

    // Send complete sensor data package to Playdate
    void sendSensorData() {
        int irRight = readIRSensor(IR_SENSOR_RIGHT_PIN);
        int irLeft = readIRSensor(IR_SENSOR_LEFT_PIN);
        float tofFront = readTofSensor(FRONT_SENSOR);
        Wire.flush(); // Clear I2C bus
        float tofBack = readTofSensor(BACK_SENSOR);
        long encRight = encoderRight.read();
        long encLeft = encoderLeft.read();
        int lightValue = analogRead(LIGHT_SENSOR_PIN);
        float batteryVoltage = batteryManager.getVoltage();
        float distanceInMeters = distanceTracker.getDistanceMeters();

        char buffer[120];
        snprintf(buffer, sizeof(buffer), 
                "msg d/%d/%d/%.2f/%.2f/%ld/%ld/%d/%.2f/%d/%.2f", 
                irRight, irLeft, tofFront, tofBack, encRight, encLeft, lightValue, 
                batteryVoltage, batteryManager.isCharging() ? 1 : 0, distanceInMeters);
        userial.println(buffer);
        DEBUG_PRINT(DEBUG_INFO, "Sent sensor data with distance: " + String(buffer));
    }

    // Get current sensor states
    float getTOFSensorFront() const { return TOFsensorFront; }
    float getTOFSensorBack() const { return TOFsensorBack; }
    bool getIsInDarkness() const { return isInDarkness; }
};

#endif // SENSOR_MANAGER_H