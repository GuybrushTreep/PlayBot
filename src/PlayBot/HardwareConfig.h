// HardwareConfig.h
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "PIDConfig.h"
// ================= Pin Assignments =================
// Motors
#define MOTOR1_PWM_PIN 7   // Left motor PWM
#define MOTOR1_DIR_PIN 6   // Left motor direction
#define MOTOR2_PWM_PIN 9   // Right motor PWM  
#define MOTOR2_DIR_PIN 8   // Right motor direction

// Encoders 
#define ENC1_PIN_A 4      // Left encoder A
#define ENC1_PIN_B 5      // Left encoder B
#define ENC2_PIN_A 3      // Right encoder A 
#define ENC2_PIN_B 2      // Right encoder B

// Sensors
#define IR_SENSOR_LEFT_PIN 39
#define IR_SENSOR_RIGHT_PIN 38
#define LIGHT_SENSOR_PIN 27
#define TOF_MULTIPLEXER_ADDR 0x70  // I2C address for ToF multiplexer

// Status & Control
#define LED_PIN 28
#define LED_COUNT 1
#define SERVO_PIN 23
#define SD_CS_PIN BUILTIN_SDCARD

// Battery Detection
#define USB_DETECT_PIN 17   
#define VOLTAGE_DIVIDER_RATIO (2.0/3.0)  // Voltage divider (2k/(1k+2k))

// ================= Hardware Constants =================
// Physical dimensions (mm)
#define WHEEL_DIAMETER 33.5f
#define WHEEL_BASE 81.0f     // Distance between wheels
#define ENCODER_PPR 813      // Pulses per revolution

#define MM_PER_TICK ((PI * WHEEL_DIAMETER) / ENCODER_PPR)
#define TICKS_PER_ROBOT_ROTATION (int)((PI * WHEEL_BASE * ENCODER_PPR) / WHEEL_DIAMETER)

// Motor characteristics
#define MOTOR_PWM_FREQUENCY 330000  // PWM frequency in Hz
#define MOTOR2_POWER_COMPENSATION 1.0f  // Compensation factor for motor 2
#define MOTOR1_POWER_COMPENSATION 1.0f  // Compensation factor for motor 1

// Include libraries 
#include <Encoder.h>
#include <DRV8835MotorShield.h>
#include <PID_v1.h>
#undef REVERSE  // Undefine REVERSE before PID
#include <WS2812FX.h>
#include <Servo.h>
#include <USBHost_t36.h>
#include "Adafruit_MAX1704X.h" 
#include <PCA9540BD.h>

// ================= Hardware Objects & Variables =================
// PID variables with inline initialization
inline double Input = 0, Output = 0, Setpoint = 0;
inline double Input2 = 0, Output2 = 0, Setpoint2 = 0;

// Core hardware objects
inline Encoder myEnc(ENC1_PIN_A, ENC1_PIN_B);
inline Encoder myEnc2(ENC2_PIN_A, ENC2_PIN_B);
inline DRV8835MotorShield motors(MOTOR1_PWM_PIN, MOTOR1_DIR_PIN, MOTOR2_PWM_PIN, MOTOR2_DIR_PIN);

// PID controllers
inline PID myPID(&Input, &Output, &Setpoint, PID_KP, PID_KI, PID_KD, AUTOMATIC);
inline PID myPID2(&Input2, &Output2, &Setpoint2, PID_KP, PID_KI, PID_KD, AUTOMATIC);

// Peripheral devices
inline WS2812FX ws2812fx(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
inline Servo headServo;
inline USBHost myusb;
inline USBHub hub1(myusb);
inline USBSerial_BigBuffer userial(myusb, 1);
inline PCA9540BD mux;
inline Adafruit_MAX17048 maxlipo;

#endif // HARDWARE_CONFIG_H