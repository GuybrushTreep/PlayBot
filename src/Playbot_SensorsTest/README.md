# PlayBot Sensor Test Program

A diagnostic tool for testing and calibrating PlayBot's hardware components.

## Hardware Requirements

- Teensy microcontroller
- DRV8835 motor driver
- 2x Quadrature encoders
- 2x IR sensors
- 2x ToF sensors with PCA9540BD multiplexer
- Light sensor
- MAX17048 battery gauge
- Servo motor for head movement

## Features

- Real-time sensor monitoring
- Motor movement testing
- Servo sweep testing
- Battery monitoring

## Commands

| Command | Description |
|---------|-------------|
| `s` | Start continuous sensor monitoring |
| `t` | Test motors (forward/backward) |
| `v` | Test servo sweep movement |
| `c` | Center servo position |
| `h` | Display help message |

## Installation

1. Install required libraries:
   - Encoder
   - Adafruit_MAX1704X
   - PCA9540BD
   - DRV8835MotorShield
   - Servo

2. Upload to Teensy using Arduino IDE

## Usage

1. Open Serial Monitor (115200 baud)
2. Send 's' to start monitoring
3. Use commands to test specific components

