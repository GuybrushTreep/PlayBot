// MotorController.h
#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "Config.h"
#include "Debug.h"
#include "HardwareConfig.h"

// Manages robot motors, encoders and PID control
// Handles:
// - Direct motor control for animations
// - PID-controlled precise movements
// - Robot rotation (triggered by Playdate "t/turns/direction")
// - Motor safety and power management
class MotorController {
private:
    // Hardware references
    DRV8835MotorShield& motors;     // Motor driver
    Encoder& encoderRight;          // Right motor encoder
    Encoder& encoderLeft;           // Left motor encoder
    PID& pidRight;                  // Right motor PID controller
    PID& pidLeft;                   // Left motor PID controller
    double& inputRight;             // Right encoder input
    double& inputLeft;              // Left encoder input
    double& setpointRight;          // Right motor target
    double& setpointLeft;           // Left motor target
    double& outputRight;            // Right motor output
    double& outputLeft;             // Left motor output
    long prevT;                     // Previous update time

public:
    // Initialize controller with all required components
    MotorController(
        DRV8835MotorShield& motorsRef,
        Encoder& encRight,
        Encoder& encLeft,
        PID& pidRight,
        PID& pidLeft,
        double& setpRight,
        double& setpLeft,
        double& inRight,
        double& inLeft,
        double& outRight,
        double& outLeft
    ) : motors(motorsRef),
        encoderRight(encRight),
        encoderLeft(encLeft),
        pidRight(pidRight),
        pidLeft(pidLeft),
        inputRight(inRight),
        inputLeft(inLeft),
        setpointRight(setpRight),
        setpointLeft(setpLeft),
        outputRight(outRight),
        outputLeft(outLeft),
        prevT(0)
    {
    }

    // Initialize motor hardware and PID controllers
    void initialize() {
        DEBUG_PRINT(DEBUG_INFO, "Initializing motors and PID controllers");
        
        // Configure motor PWM frequency to reduce audible noise
        analogWriteFrequency(MOTOR1_PWM_PIN, MOTOR_PWM_FREQUENCY);
        analogWriteFrequency(MOTOR1_DIR_PIN, MOTOR_PWM_FREQUENCY);
        analogWriteFrequency(MOTOR2_PWM_PIN, MOTOR_PWM_FREQUENCY);
        analogWriteFrequency(MOTOR2_DIR_PIN, MOTOR_PWM_FREQUENCY);
        
        // Set motor directions (LEFT motor is flipped)
        motors.flipM1(true); // LEFT
        motors.flipM2(false);
        
        // Configure PID controllers for both motors
        pidRight.SetTunings(PID_KP, PID_KI, PID_KD);
        pidRight.SetOutputLimits(-1023, 1023);
        pidRight.SetMode(AUTOMATIC);
        pidRight.SetSampleTime(1);

        pidLeft.SetTunings(PID_KP, PID_KI, PID_KD);
        pidLeft.SetOutputLimits(-1023, 1023);
        pidLeft.SetMode(AUTOMATIC);
        pidLeft.SetSampleTime(1);
        
        DEBUG_PRINT(DEBUG_INFO, "Motors and PID controllers initialized");
    }

    // Update timing variables for PID calculations
    void updateTimers() {
        long currT = micros();
        prevT = currT;
        DEBUG_PRINT(DEBUG_VERBOSE, "Timers updated");
    }

    // Read current encoder positions
    void updateEncoders() {
        inputRight = encoderRight.read();
        inputLeft = encoderLeft.read();
        DEBUG_PRINT(DEBUG_VERBOSE, "Encoders updated: InputRight=" + String(inputRight) + 
                                 ", InputLeft=" + String(inputLeft));
    }

    // Update motor target positions from animation commands
    void updateSetpoints(const String& rightWheel, const String& leftWheel, bool isAnimationPlaying) {
        if (isAnimationPlaying) {
            setpointRight = rightWheel.toFloat();
            setpointLeft = leftWheel.toFloat();
        }
        DEBUG_PRINT(DEBUG_VERBOSE, "Setpoints updated: SetpointRight=" + String(setpointRight) + 
                    ", SetpointLeft=" + String(setpointLeft));
    }

    // Apply motor speeds based on PID output
    // Handles motion enable/disable and motor compensation
    void controlMotors(bool isAnimationPlaying, bool motionEnabled) {
        if (setpointRight == 0) {  
            motors.setM1Speed(0);
            motors.setM2Speed(0);
            DEBUG_PRINT(DEBUG_VERBOSE, "Motors stopped - zero setpoint");
            return;
        }

        if (isAnimationPlaying) {
            if (!motionEnabled) {
                motors.setM1Speed(0);
                motors.setM2Speed(0);
                DEBUG_PRINT(DEBUG_VERBOSE, "Motors stopped - Motion disabled during animation");
            } else {
                motors.setM1Speed(outputLeft);   // Left motor (M1)
                // Apply power compensation to right motor 
                motors.setM2Speed(outputRight * MOTOR2_POWER_COMPENSATION);
                DEBUG_PRINT(DEBUG_VERBOSE, "Motors speed set: M1=" + String(outputLeft) + 
                           ", M2=" + String(outputRight * MOTOR2_POWER_COMPENSATION));
            }
        }
    }

    // Calculate new PID outputs
    void computePID() {
        pidRight.Compute();
        pidLeft.Compute();
        DEBUG_PRINT(DEBUG_VERBOSE, "PID computed: OutputRight=" + String(outputRight) + 
                    ", OutputLeft=" + String(outputLeft));
    }

    // Rotate robot in response to Playdate crank turns
    // Sends "msg r/1" when rotation complete
    void rotateRobot(int numberOfTurns, int direction) {
        // Reset encoders for accurate rotation
        encoderRight.write(0);
        encoderLeft.write(0);
        
        // Calculate required encoder ticks for rotation
        // One 360° turn requires 1854 ticks (2.21 wheel rotations)
        const int TICKS_FOR_360 = 1854;
        long targetTicks = TICKS_FOR_360 * numberOfTurns;
        
        DEBUG_PRINT(DEBUG_INFO, "Starting rotation - Direction: " + String(direction) + 
                    " Target: " + String(targetTicks));
        
        // Set motor speeds with direction
        int m2_speed = direction > 0 ? ROTATION_SPEED : -ROTATION_SPEED;
        int m1_speed = direction > 0 ? 
            -ROTATION_SPEED * MOTOR2_POWER_COMPENSATION : 
            ROTATION_SPEED * MOTOR2_POWER_COMPENSATION;
        
        motors.setM1Speed(m1_speed);
        motors.setM2Speed(m2_speed);
        
        // Wait for target rotation
        while(abs(encoderRight.read()) < targetTicks) {
            delay(1);
        }
        
        // Notify Playdate rotation is complete
        userial.println("msg r/1");
        
        // Stop and reset
        motors.setM1Speed(0);
        motors.setM2Speed(0);
        encoderRight.write(0);
        encoderLeft.write(0);
        DEBUG_PRINT(DEBUG_INFO, "Rotation complete - Final positions E1: " + 
                    String(encoderRight.read()) + " E2: " + String(encoderLeft.read()));
    }
};

#endif // MOTOR_CONTROLLER_H