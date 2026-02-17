/**
 * Rally Telemetry Pro - Advanced Configuration
 * 
 * FreeRTOS-based high-performance telemetry system for rally cars.
 * Features: dual-core processing, real-time alerts, WiFi streaming, binary logging
 */

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// =============================================================================
// VERSION
// =============================================================================

#define FIRMWARE_VERSION "2.0.0-RTOS"
#define FIRMWARE_NAME "Rally Telemetry Pro"

// =============================================================================
// RTOS CONFIGURATION
// =============================================================================

// Task priorities (higher = more urgent)
#define TASK_PRIORITY_SENSOR    configMAX_PRIORITIES - 1  // Highest - critical timing
#define TASK_PRIORITY_ALERT     configMAX_PRIORITIES - 2  // High - safety critical
#define TASK_PRIORITY_LOGGING   configMAX_PRIORITIES - 3  // Medium - data persistence
#define TASK_PRIORITY_TELEMETRY configMAX_PRIORITIES - 4  // Low - network streaming
#define TASK_PRIORITY_STATUS    configMAX_PRIORITIES - 5  // Lowest - UI updates

// Task stack sizes (words)
#define STACK_SIZE_SENSOR     4096
#define STACK_SIZE_LOGGING    8192
#define STACK_SIZE_TELEMETRY  4096
#define STACK_SIZE_ALERT      4096
#define STACK_SIZE_STATUS     2048
#define STACK_SIZE_COMPUTE    4096

// Task core assignments (ESP32 has Core 0 and Core 1)
#define CORE_SENSOR    0  // Core 0: Real-time sensor reading
#define CORE_COMPUTE   0  // Core 0: Data processing
#define CORE_LOGGING   1  // Core 1: SD card (blocking I/O)
#define CORE_TELEMETRY 1  // Core 1: Network (blocking I/O)
#define CORE_STATUS    1  // Core 1: LED/status

// =============================================================================
// TIMING CONSTANTS
// =============================================================================

// Sampling rates (Hz)
const uint32_t IMU_SAMPLE_RATE_HZ = 100;      // 100Hz IMU (high-res for vibration analysis)
const uint32_t GPS_SAMPLE_RATE_HZ = 10;       // 10Hz GPS (standard for NEO-M8N)
const uint32_t LOG_RATE_HZ = 50;              // 50Hz combined logging
const uint32_t TELEMETRY_RATE_HZ = 20;        // 20Hz WiFi streaming

// Calculate intervals in milliseconds
const TickType_t IMU_INTERVAL_MS = pdMS_TO_TICKS(1000 / IMU_SAMPLE_RATE_HZ);
const TickType_t GPS_INTERVAL_MS = pdMS_TO_TICKS(1000 / GPS_SAMPLE_RATE_HZ);
const TickType_t LOG_INTERVAL_MS = pdMS_TO_TICKS(1000 / LOG_RATE_HZ);
const TickType_t TELEMETRY_INTERVAL_MS = pdMS_TO_TICKS(1000 / TELEMETRY_RATE_HZ);

// =============================================================================
// BUFFER CONFIGURATION
// =============================================================================

// Ring buffer sizes (must be power of 2 for efficient masking)
const size_t IMU_BUFFER_SIZE = 256;           // ~2.5 seconds at 100Hz
const size_t GPS_BUFFER_SIZE = 32;            // ~3 seconds at 10Hz
const size_t LOG_BUFFER_SIZE = 128;           // ~2.5 seconds at 50Hz
const size_t ALERT_QUEUE_SIZE = 16;           // Alert queue depth
const size_t TELEMETRY_BUFFER_SIZE = 64;      // Network queue

// =============================================================================
// SERIAL CONFIGURATION
// =============================================================================

const long SERIAL_BAUD = 115200;

// Debug levels: 0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE
#define DEBUG_LEVEL 4

#define DEBUG_PRINT(level, x)   do { if (DEBUG_LEVEL >= level) Serial.print(x); } while(0)
#define DEBUG_PRINTLN(level, x) do { if (DEBUG_LEVEL >= level) Serial.println(x); } while(0)
#define DEBUG_PRINTF(level, ...) do { if (DEBUG_LEVEL >= level) Serial.printf(__VA_ARGS__); } while(0)

// =============================================================================
// PIN ASSIGNMENTS
// =============================================================================

// Status LEDs (RGB for detailed status)
const int LED_PIN_RED   = 25;
const int LED_PIN_GREEN = 26;
const int LED_PIN_BLUE  = 27;

// GPS Module (UART2)
const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;

// SD Card (SPI)
const int SD_MOSI_PIN = 23;
const int SD_MISO_PIN = 19;
const int SD_SCK_PIN  = 18;
const int SD_CS_PIN   = 5;

// I2C (MPU6050)
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// CAN Bus (optional - for OBD-II integration)
const int CAN_RX_PIN = 4;
const int CAN_TX_PIN = 15;

// =============================================================================
// SENSOR CONFIGURATION
// =============================================================================

// GPS
const int GPS_BAUD_RATE = 9600;
const int GPS_BUFFER_SIZE_BYTES = 256;

// IMU (MPU6050)
const uint8_t MPU6050_ADDR = 0x68;
const float ACCEL_SCALE = 16384.0f;
const float GYRO_SCALE = 131.0f;

// =============================================================================
// STORAGE CONFIGURATION
// =============================================================================

// Binary log format (more efficient than CSV)
#define USE_BINARY_FORMAT true

// Log rotation
const uint32_t MAX_LOG_SIZE_BYTES = 50 * 1024 * 1024;  // 50MB per file
const uint32_t MAX_LOG_FILES = 10;
const char* LOG_FILE_BASE = "/rally";
const char* LOG_EXT = ".bin";

// SD flush settings
const int FLUSH_INTERVAL_WRITES = 100;
const uint32_t FLUSH_INTERVAL_MS = 5000;

// =============================================================================
// ALERT THRESHOLDS
// =============================================================================

// G-force limits (for rally racing context)
const float ALERT_G_FORCE_WARN  = 2.5f;   // Warning at 2.5G
const float ALERT_G_FORCE_CRIT  = 3.5f;   // Critical at 3.5G
const float ALERT_G_FORCE_MAX   = 5.0f;   // Max recorded in WRC is ~4.5G

// Temperature limits
const float ALERT_TEMP_WARN  = 60.0f;   // IMU warning (deg C)
const float ALERT_TEMP_CRIT  = 75.0f;   // IMU critical

// Roll/pitch limits (degrees)
const float ALERT_ROLL_WARN  = 25.0f;
const float ALERT_ROLL_CRIT  = 35.0f;
const float ALERT_PITCH_WARN = 20.0f;
const float ALERT_PITCH_CRIT = 30.0f;

// =============================================================================
// TELEMETRY CONFIGURATION
// =============================================================================

// WiFi AP Mode settings
const char* WIFI_AP_SSID = "RallyTelemetry";
const char* WIFI_AP_PASS = "rally2024";
const IPAddress WIFI_AP_IP(192, 168, 4, 1);
const IPAddress WIFI_AP_NETMASK(255, 255, 255, 0);

// UDP Streaming
const uint16_t TELEMETRY_UDP_PORT = 5005;
const char* TELEMETRY_UDP_HOST = "192.168.4.255";  // Broadcast

// Web server
const uint16_t WEB_SERVER_PORT = 80;

// =============================================================================
// DATA STRUCTURES
// =============================================================================

// Packed IMU data sample (16 bytes)
struct __attribute__((packed)) IMUData {
    uint32_t timestamp_ms;    // 4 bytes
    float accel_x;            // 4 bytes (m/s^2)
    float accel_y;            // 4 bytes
    float accel_z;            // 4 bytes
    float gyro_x;             // 4 bytes (deg/s)
    float gyro_y;             // 4 bytes
    float gyro_z;             // 4 bytes
    float temperature;        // 4 bytes (Celsius)
};

// Packed GPS data sample (28 bytes)
struct __attribute__((packed)) GPSData {
    uint32_t timestamp_ms;    // 4 bytes
    double latitude;          // 8 bytes
    double longitude;         // 8 bytes
    float altitude;           // 4 bytes (meters)
    float speed_kmh;          // 4 bytes
    float heading;            // 4 bytes (degrees)
    uint8_t satellites;       // 1 byte
    uint8_t fix_quality;      // 1 byte
    uint8_t hdop;             // 1 byte (x10 for precision)
    uint8_t padding;          // 1 byte (alignment)
};

// Combined telemetry packet (64 bytes) - for logging/streaming
struct __attribute__((packed)) TelemetryPacket {
    uint32_t magic;           // 4 bytes - 'RALLY' = 0x52414C4C
    uint16_t version;         // 2 bytes - protocol version
    uint16_t sequence;        // 2 bytes - packet sequence
    uint32_t timestamp_ms;    // 4 bytes
    IMUData imu;              // 28 bytes (subset)
    GPSData gps;              // 28 bytes (subset)
    uint16_t crc16;           // 2 bytes - checksum
};

// Alert types
enum class AlertType : uint8_t {
    NONE = 0,
    GFORCE_WARNING,
    GFORCE_CRITICAL,
    TEMP_WARNING,
    TEMP_CRITICAL,
    ROLL_WARNING,
    ROLL_CRITICAL,
    PITCH_WARNING,
    PITCH_CRITICAL,
    GPS_LOST,
    SD_ERROR,
    LOW_BATTERY
};

// Alert structure
struct Alert {
    AlertType type;
    uint32_t timestamp_ms;
    float value;
    float threshold;
};

// System state
enum class SystemState : uint8_t {
    INITIALIZING = 0,
    CALIBRATING,
    READY,
    RECORDING,
    ERROR,
    SHUTDOWN
};

// =============================================================================
// CONSTANTS
// =============================================================================

const float GRAVITY_MS2 = 9.80665f;
const float PI_F = 3.14159265359f;
const uint32_t PACKET_MAGIC = 0x52414C4C;  // "RALL"
const uint16_t PACKET_VERSION = 2;

// =============================================================================
// COMPILE-TIME VALIDATION
// =============================================================================

static_assert(sizeof(IMUData) == 32, "IMUData struct size mismatch");
static_assert(sizeof(GPSData) == 32, "GPSData struct size mismatch");
static_assert(sizeof(TelemetryPacket) == 72, "TelemetryPacket struct size mismatch");

#if LOG_RATE_HZ > IMU_SAMPLE_RATE_HZ
    #error "LOG_RATE_HZ cannot exceed IMU_SAMPLE_RATE_HZ"
#endif
