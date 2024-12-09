// Config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "PIDConfig.h"
#include "HardwareConfig.h"

// ================= Communication Settings =================
#define USBBAUD 115200
#define USB_BUFFER_SIZE 512

// ================= Light Sensor Configuration =================
#define LIGHT_CHECK_INTERVAL 200      // ms
#define DARKNESS_THRESHOLD 10

// ================= Edge Detection Configuration =================
#define EDGE_THRESHOLD 170            // mm
#define FRONT_SENSOR 1
#define BACK_SENSOR 0
#define EDGE_CHECK_INTERVAL 12        // ms

// ================= IR Sensor Configuration =================
#define IR_CHECK_INTERVAL 8           // ms

// ================= Battery Monitoring Configuration =================
#define BATTERY_CHECK_INTERVAL 250    // ms
// Battery thresholds
#define BATTERY_LOW_THRESHOLD 15.0f     // 15% battery threshold for warning
#define BATTERY_CRITICAL_THRESHOLD 5.0f  // 5% battery threshold for shutdown
// ================= Collision Detection Configuration =================
#define FRONT_COLLISION_THRESHOLD 70  // mm
#define COLLISION_CHECK_INTERVAL 4   // ms

// ================= Motion Constants =================

#define ROTATION_SPEED 200
extern bool MOTION_ENABLED;  // Global flag to control all motor movements
// ================= Distance Tracking =================
#define DISTANCE_UPDATE_INTERVAL 100   // ms
#define DISTANCE_LOG_INTERVAL 10000    // ms


#endif // CONFIG_H