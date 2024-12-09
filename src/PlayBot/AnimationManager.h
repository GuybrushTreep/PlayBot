// AnimationManager.h
#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "Config.h"
#include "Debug.h"
#include <SD.h>

// Manages robot animations loaded from SD card
// Animations are triggered by Playdate messages:
// - "a/filepath" : Start animation from SD file
// - "x" : Stop current animation
// Each animation frame contains: index/head_position/right_wheel/left_wheel
class AnimationManager {
private:
    // Static buffer sizes for memory efficiency
    static const size_t BUFFER_SIZE = 512;     // SD card read buffer
    static const uint8_t MAX_LINE_LENGTH = 64; // Max animation line length
    
    // Static buffers for string operations
    char buffer[BUFFER_SIZE];      // Main file reading buffer
    char lineBuf[MAX_LINE_LENGTH]; // Current line buffer
    char wheelRightBuf[8];         // Right wheel command buffer
    char wheelLeftBuf[8];          // Left wheel command buffer
    
    // File and animation state
    File currentFile;              // Current animation file handle
    const char* currentAnimation;  // Path to current animation
    size_t bufferPos;             // Position in read buffer
    size_t bytesInBuffer;         // Valid bytes in buffer
    bool isPlaying;               // Animation playback state
    uint32_t lastFrameTime;       // Last frame timestamp
    
    // Hardware references
    Servo& headServo;             // Head servo control
    DRV8835MotorShield& motors;   // Motor control
    Encoder& encoderLeft;         // Left wheel encoder
    Encoder& encoderRight;        // Right wheel encoder

    // Fast integer parsing without String conversion
    inline int32_t parseIntFast(const char* str, size_t len) {
        int32_t result = 0;
        bool negative = false;
        size_t i = 0;

        if (str[0] == '-') {
            negative = true;
            i++;
        }

        for (; i < len && str[i] >= '0' && str[i] <= '9'; i++) {
            result = result * 10 + (str[i] - '0');
        }

        return negative ? -result : result;
    }

    // Optimized line reading from SD card
    bool readNextLine() {
        size_t lineLen = 0;
        
        while (lineLen < MAX_LINE_LENGTH - 1) {
            // Refill buffer if needed
            if (bufferPos >= bytesInBuffer) {
                bytesInBuffer = currentFile.read(buffer, BUFFER_SIZE);
                bufferPos = 0;
                if (bytesInBuffer == 0) return false;
            }
            
            char c = buffer[bufferPos++];
            if (c == '\n') {
                lineBuf[lineLen] = '\0';
                return true;
            }
            if (c != '\r') {
                lineBuf[lineLen++] = c;
            }
        }
        
        lineBuf[lineLen] = '\0';
        return true;
    }

    // Parse a single animation frame line
    // Format: index/head_position/right_wheel/left_wheel
    void parseAnimationLine() {
        char* ptr = lineBuf;
        char* nextSlash;
        
        // Skip frame index
        nextSlash = strchr(ptr, '/');
        if (!nextSlash) return;
        ptr = nextSlash + 1;
        
        // Parse head servo position (500-2500 µs)
        nextSlash = strchr(ptr, '/');
        if (!nextSlash) return;
        *nextSlash = '\0';
        int headPos = parseIntFast(ptr, nextSlash - ptr);
        ptr = nextSlash + 1;
        
        // Parse right wheel speed
        nextSlash = strchr(ptr, '/');
        if (!nextSlash) return;
        size_t len = nextSlash - ptr;
        if (len < sizeof(wheelRightBuf)) {
            memcpy(wheelRightBuf, ptr, len);
            wheelRightBuf[len] = '\0';
        }
        ptr = nextSlash + 1;
        
        // Parse left wheel speed
        len = strlen(ptr);
        if (len < sizeof(wheelLeftBuf)) {
            memcpy(wheelLeftBuf, ptr, len + 1);
        }
        
        // Apply head position if motion is enabled
        if (MOTION_ENABLED && headPos >= 500 && headPos <= 2500) {
            headServo.writeMicroseconds(headPos);
        }
    }

public:
    // Initialize manager with required hardware references
    AnimationManager(Servo& servo, DRV8835MotorShield& motorController, 
                    Encoder& encLeft, Encoder& encRight)
        : headServo(servo)
        , motors(motorController)
        , encoderLeft(encLeft)
        , encoderRight(encRight)
        , isPlaying(false)
        , lastFrameTime(0)
        , currentAnimation(nullptr)
        , bufferPos(0)
        , bytesInBuffer(0) {
        // Initialize wheel command buffers to "0"
        wheelRightBuf[0] = '0';
        wheelRightBuf[1] = '\0';
        wheelLeftBuf[0] = '0';
        wheelLeftBuf[1] = '\0';
    }

    // Check if animation is currently playing
    bool isAnimationPlaying() const { 
        return isPlaying; 
    }

    // Start new animation from SD card
    // Called when Playdate sends "a/filepath" message
    void startAnimation(const char* animationPath) {
        if (isPlaying) stopAnimation();
        
        currentAnimation = animationPath;
        currentFile = SD.open(animationPath);
        
        if (!currentFile) {
            DEBUG_PRINT(DEBUG_WARNING, "Failed to open animation");
            return;
        }
        
        isPlaying = true;
        bufferPos = bytesInBuffer = 0;
        encoderLeft.write(0);
        encoderRight.write(0);
        
        if (MOTION_ENABLED) {
            headServo.attach(SERVO_PIN);
        }
    }

    // Stop current animation playback
    // Called when Playdate sends "x" message
    void stopAnimation() {
        if (!isPlaying) return;
        
        headServo.detach();
        currentFile.close();
        encoderLeft.write(0);
        encoderRight.write(0);
        motors.setM1Speed(0);
        motors.setM2Speed(0);
        isPlaying = false;
        // Reset wheel commands to "0"
        wheelRightBuf[0] = '0';
        wheelRightBuf[1] = '\0';
        wheelLeftBuf[0] = '0';
        wheelLeftBuf[1] = '\0';
        bufferPos = bytesInBuffer = 0;
    }

    // Process animation frame - called in main loop
    // Maintains ~30fps timing (33ms per frame)
    void update() {
        if (!isPlaying) return;

        uint32_t currentTime = millis();
        if (currentTime - lastFrameTime > 33) {
            if (currentFile && readNextLine()) {
                parseAnimationLine();
                lastFrameTime = currentTime;
            } else {
                stopAnimation();
            }
        }
    }

    // Get current wheel commands for motor controller
    void getWheelCommands(String& right, String& left) {
        right = wheelRightBuf;
        left = wheelLeftBuf;
    }
};

#endif // ANIMATION_MANAGER_H