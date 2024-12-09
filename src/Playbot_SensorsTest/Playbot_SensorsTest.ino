#include <Wire.h>
#include <Encoder.h>
#include "Adafruit_MAX1704X.h"
#include <PCA9540BD.h>
#include <DRV8835MotorShield.h>
#include <Servo.h>

// Pin definitions from HardwareConfig.h
#define IR_SENSOR_LEFT_PIN 39
#define IR_SENSOR_RIGHT_PIN 38
#define LIGHT_SENSOR_PIN 27
#define TOF_MULTIPLEXER_ADDR 0x70
#define ENC1_PIN_A 4
#define ENC1_PIN_B 5
#define ENC2_PIN_A 3
#define ENC2_PIN_B 2
#define MOTOR1_PWM_PIN 7   // Left motor PWM
#define MOTOR1_DIR_PIN 6   // Left motor direction
#define MOTOR2_PWM_PIN 9   // Right motor PWM  
#define MOTOR2_DIR_PIN 8   // Right motor direction
#define SERVO_PIN 23
#define SERVO_CENTER 1500
#define SERVO_SWEEP_AMOUNT 200
#define SERVO_DELAY 15

// Motor test parameters
#define MOTOR_SPEED 200
#define TEST_TICKS 840

// Welcome message
const char* WELCOME_MESSAGE = R"(
=================================================
         PLAYBOT TEST & DIAGNOSTIC PROGRAM         
=================================================
This program tests all sensors and motors:

SENSORS:
- IR sensors (left & right)
- Light sensor
- ToF sensors (front & back)
- Encoders (left & right)
- Battery voltage & percentage

MOTORS TEST (when sending 't'):
- Forward motion: 840 ticks
- Backward motion: 840 ticks

SERVO TEST:
'v' - Sweep servo head left/right
'c' - Center servo head

COMMANDS:
's' - Start continuous sensor monitoring
't' - Test motors movement
'v' - Test servo sweep
'c' - Center servo
'h' - Show this help message

Please send 's' to start the program...
=================================================
)";

// Initialize objects
Encoder encLeft(ENC1_PIN_A, ENC1_PIN_B);
Encoder encRight(ENC2_PIN_A, ENC2_PIN_B);
Adafruit_MAX17048 maxlipo;
PCA9540BD mux;
DRV8835MotorShield motors(MOTOR1_PWM_PIN, MOTOR1_DIR_PIN, MOTOR2_PWM_PIN, MOTOR2_DIR_PIN);
Servo headServo;

// ToF variables
unsigned char i2c_rx_buf[16];
unsigned short lenth_val = 0;

void showHelp() {
  Serial.println(WELCOME_MESSAGE);
}

// Read ToF sensor helper function
int ReadDistance() {
  Wire.beginTransmission(82);
  Wire.write(byte(0x00));
  Wire.endTransmission();
  Wire.requestFrom(82, 2);
  if (2 <= Wire.available()) {
    i2c_rx_buf[0] = Wire.read();
    i2c_rx_buf[1] = Wire.read();
    lenth_val = (i2c_rx_buf[0] << 8) | i2c_rx_buf[1];
    return lenth_val;
  }
  return 0;
}

void centerServo() {
  Serial.println("Centering servo...");
  headServo.writeMicroseconds(SERVO_CENTER);
  delay(500);
  Serial.println("Servo centered");
}

void sweepServo() {
  Serial.println("Testing servo sweep...");
  
  // Sweep right
  for(int pos = SERVO_CENTER; pos <= SERVO_CENTER + SERVO_SWEEP_AMOUNT; pos += 5) {
    headServo.writeMicroseconds(pos);
    Serial.print("Servo position: ");
    Serial.println(pos);
    delay(SERVO_DELAY);
  }
  
  // Small pause at end position
  delay(500);
  
  // Sweep left
  for(int pos = SERVO_CENTER + SERVO_SWEEP_AMOUNT; pos >= SERVO_CENTER - SERVO_SWEEP_AMOUNT; pos -= 5) {
    headServo.writeMicroseconds(pos);
    Serial.print("Servo position: ");
    Serial.println(pos);
    delay(SERVO_DELAY);
  }
  
  // Small pause at end position
  delay(500);
  
  // Return to center
  for(int pos = SERVO_CENTER - SERVO_SWEEP_AMOUNT; pos <= SERVO_CENTER; pos += 5) {
    headServo.writeMicroseconds(pos);
    Serial.print("Servo position: ");
    Serial.println(pos);
    delay(SERVO_DELAY);
  }
  
  Serial.println("Sweep complete");
}

// Motor test function
void testMotors() {
  // Reset encoders
  encLeft.write(0);
  encRight.write(0);
  
  // Forward motion
  Serial.println("Moving forward...");
  while(abs(encLeft.read()) < TEST_TICKS) {
    motors.setM1Speed(MOTOR_SPEED);  // Left motor
    motors.setM2Speed(MOTOR_SPEED);  // Right motor
    Serial.print("Encoder Left: ");
    Serial.println(encLeft.read());
  }
  
  // Stop
  motors.setM1Speed(0);
  motors.setM2Speed(0);
  delay(1000);
  
  // Reset encoders
  encLeft.write(0);
  encRight.write(0);
  
  // Backward motion
  Serial.println("Moving backward...");
  while(abs(encLeft.read()) < TEST_TICKS) {
    motors.setM1Speed(-MOTOR_SPEED);  // Left motor
    motors.setM2Speed(-MOTOR_SPEED);  // Right motor
    Serial.print("Encoder Left: ");
    Serial.println(encLeft.read());
  }
  
  // Stop
  motors.setM1Speed(0);
  motors.setM2Speed(0);
  Serial.println("Test complete");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Show welcome message
  showHelp();
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize battery gauge
  if (!maxlipo.begin()) {
    Serial.println("Battery gauge not found!");
  }
  
  // Initialize IR sensors
  pinMode(IR_SENSOR_LEFT_PIN, INPUT_DISABLE);
  pinMode(IR_SENSOR_RIGHT_PIN, INPUT_DISABLE);
  
  // Configure motors
  analogWriteFrequency(MOTOR1_PWM_PIN, 330000);
  analogWriteFrequency(MOTOR1_DIR_PIN, 330000);
  analogWriteFrequency(MOTOR2_PWM_PIN, 330000);
  analogWriteFrequency(MOTOR2_DIR_PIN, 330000);
  motors.flipM1(true);
  
  // Initialize servo
  headServo.attach(SERVO_PIN);
  centerServo();  // Center servo at startup
  
  // Wait for user input to start
  bool started = false;
  while (!started) {
    if (Serial.available() > 0) {
      char cmd = Serial.read();
      if (cmd == 's') {
        started = true;
        Serial.println("\nStarting sensor monitoring...\n");
      } else if (cmd == 'h') {
        showHelp();
      }
    }
    delay(100);
  }
}

void loop() {
  // Check for serial commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    switch(cmd) {
      case 't':
        testMotors();
        break;
      case 'h':
        showHelp();
        break;
      case 'v':
        sweepServo();
        break;
      case 'c':
        centerServo();
        break;
    }
  }

  // Read and print IR sensors
  Serial.println("\n=== IR Sensors ===");
  Serial.print("Left IR: ");
  Serial.println(analogRead(IR_SENSOR_LEFT_PIN));
  Serial.print("Right IR: ");
  Serial.println(analogRead(IR_SENSOR_RIGHT_PIN));
  
  // Read and print light sensor
  Serial.println("\n=== Light Sensor ===");
  Serial.print("Light level: ");
  Serial.println(analogRead(LIGHT_SENSOR_PIN));
  
  // Read and print ToF sensors
  Serial.println("\n=== ToF Sensors ===");
  mux.selectChannel(1); // Front sensor
  Serial.print("Front ToF: ");
  Serial.println(ReadDistance());
  mux.selectChannel(0); // Back sensor
  Serial.print("Back ToF: ");
  Serial.println(ReadDistance());
  
  // Read and print encoders
  Serial.println("\n=== Encoders ===");
  Serial.print("Left encoder: ");
  Serial.println(encLeft.read());
  Serial.print("Right encoder: ");
  Serial.println(encRight.read());
  
  // Read and print battery info
  Serial.println("\n=== Battery ===");
  Serial.print("Voltage: ");
  Serial.print(maxlipo.cellVoltage(), 3);
  Serial.println("V");
  Serial.print("Percent: ");
  Serial.print(maxlipo.cellPercent(), 1);
  Serial.println("%");
  
  delay(1000); // Update every second
}