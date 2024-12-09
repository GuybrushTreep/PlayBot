# Robot Controller - Arduino Project

Firmware for a robot using Teensy microcontroller that communicates with a Playdate device.

## Core Components

### Main Program (PlayBot.ino)
- Main program loop and initialization
- Orchestrates all subsystems
- Manages communication between components

### Managers

#### AnimationManager (AnimationManager.h)
- Handles animation playback from SD card
- Controls servo movements
- Manages motor coordination for animations
- Real-time frame processing

#### BatteryManager (BatteryManager.h)
- Battery voltage monitoring via MAX17048 gauge
- Charging state detection
- Power state reporting to Playdate

#### CommunicationManager (CommunicationManager.h)
- USB serial communication with Playdate
- Message parsing and routing
- Protocol handling for all subsystems

#### LEDController (LEDController.h)
- WS2812 LED control
- Status indication

#### MotorController (MotorController.h)
- DRV8835 motor driver control
- PID-based motor control
- Encoder feedback processing
- Rotation and movement commands

#### SensorManager (SensorManager.h)
- ToF distance sensors management
- IR edge detection
- Light sensor readings
- Collision detection
- Sensor data aggregation and reporting

#### StorageManager (StorageManager.h)
- SD card initialization
- File system management

### Support Files

#### Config.h
- System-wide configuration parameters
- Pin definitions
- Timing constants
- Sensor thresholds

#### Debug.h/cpp
- Debugging infrastructure
- Error logging
- Setup error tracking
- Log buffering and transmission

#### DistanceTracker.h
- Tracks total distance traveled
- Persistent distance logging
- Movement statistics

#### HardwareConfig.h/cpp
- Hardware-specific configurations
- Pin assignments
- Physical constants
- Hardware object initialization

## Communication Protocol

Message Protocol Details:

Outgoing (Arduino -> Playdate):
- "msg b/percent/voltage/charging"
  Example: "msg b/85.20/3.7/1" (85.20% battery, 3.7V, charging)

- "msg d/irRight/irLeft/tofFront/tofBack/encRight/encLeft/light/batteryVoltage/charging/distanceM"
  Example: "msg d/100/120/150.20/160.50/-1200/1200/500/3.7/1/1.50"
  (IR values, ToF in mm, encoder ticks, light level, battery V, charging state, distance in meters)

- "msg e/1" (Edge detected)

- "msg l/0" or "msg l/1" (Darkness state change: 0=dark, 1=light)

- "msg p/0" or "msg p/1" (Power/charging state: 0=unplugged, 1=charging)

- "msg s/" (Connection confirmation)

- "msg w/1" (Collision detected)

- "msg r/1" (Rotation completed)

Incoming (Playdate -> Arduino):
- "a/filepath" (Start animation from SD card)

- "b" (Request battery status)

- "d" (Request sensor data)

- "v" (Connection verification ping)

- "t/turns/direction"
  Example: "t/2/1" (2 turns, direction 1=clockwise, -1=counterclockwise)

