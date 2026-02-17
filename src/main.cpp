/**
 * Rally Telemetry Pro - RTOS Edition
 * 
 * Advanced telemetry data logger for rally cars with FreeRTOS.
 * 
 * Architecture:
 * - Core 0: Sensor reading (IMU 100Hz, GPS 10Hz) + Data processing
 * - Core 1: SD logging + WiFi telemetry + Status LED
 * 
 * Features:
 * - 100Hz IMU sampling with hardware interrupt support
 * - 10Hz GPS with multi-sentence NMEA parsing
 * - 50Hz binary logging with automatic rotation
 * - 20Hz WiFi telemetry streaming (UDP + WebSocket)
 * - Real-time G-force, roll, pitch alerts
 * - Thread-safe ring buffers between tasks
 * - RGB LED status indication
 * - Web-based configuration and log download
 * 
 * Hardware: ESP32 DevKit, MPU6050, NEO-M8N GPS, SD Card
 * 
 * @version 2.0.0-RTOS
 * @author Rally Telemetry Team
 */

#include "core/config.h"
#include "core/SystemState.h"
#include "core/Tasks.h"
#include "sensors/imu.h"
#include "sensors/gps.h"
#include "alerts/AlertManager.h"
#include "storage/BinaryLogger.h"
#include "telemetry/WiFiTelemetry.h"
#include "utils/RingBuffer.h"

// =============================================================================
// GLOBAL OBJECTS
// =============================================================================

// System state
SystemStateManager g_systemState;

// Sensors
IMU g_imu;
GPS g_gps;

// Subsystems
AlertManager g_alertManager;
BinaryLogger g_logger;
WiFiTelemetry g_telemetry;

// Data flow ring buffers
RingBuffer<IMUData, IMU_BUFFER_SIZE> g_imuBuffer;
RingBuffer<GPSData, GPS_BUFFER_SIZE> g_gpsBuffer;
RingBuffer<TelemetryPacket, LOG_BUFFER_SIZE> g_logBuffer;

// Task parameters
TaskParameters g_taskParams;

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    // Initialize serial first for debugging
    Serial.begin(SERIAL_BAUD);
    delay(100);
    
    Serial.println("\n========================================");
    Serial.println(FIRMWARE_NAME);
    Serial.print("Version: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.println("========================================\n");
    
    // Initialize system state
    g_systemState.begin();
    
    // Initialize sensors
    Serial.println("[1/6] Initializing IMU...");
    if (!g_imu.begin()) {
        Serial.println("ERROR: IMU initialization failed!");
        g_systemState.postEvent(SystemEvent::ERROR_SENSOR);
    } else {
        Serial.println("  IMU OK");
    }
    
    Serial.println("[2/6] Initializing GPS...");
    if (!g_gps.begin()) {
        Serial.println("ERROR: GPS initialization failed!");
        g_systemState.postEvent(SystemEvent::ERROR_GPS);
    } else {
        Serial.println("  GPS OK");
    }
    
    // Calibrate IMU
    Serial.println("[3/6] Calibrating IMU (keep still)...");
    if (g_imu.performCalibration(200)) {  // 200 samples
        Serial.println("  Calibration OK");
    } else {
        Serial.println("  Calibration failed, using defaults");
    }
    
    // Initialize storage
    Serial.println("[4/6] Initializing SD card...");
    if (!g_logger.begin()) {
        Serial.println("ERROR: SD card initialization failed!");
        g_systemState.postEvent(SystemEvent::ERROR_STORAGE);
    } else {
        Serial.println("  SD OK");
    }
    
    // Initialize alert system
    Serial.println("[5/6] Initializing alert system...");
    g_alertManager.begin();
    
    // Configure alert thresholds (example: rally car settings)
    g_alertManager.setGForceThresholds(2.0f, 3.0f, 0.15f);   // 2G warn, 3G critical
    g_alertManager.setRollThresholds(20.0f, 30.0f, 0.1f);    // Degrees
    g_alertManager.setPitchThresholds(15.0f, 25.0f, 0.1f);   // Degrees
    Serial.println("  Alerts OK");
    
    // Initialize WiFi telemetry
    Serial.println("[6/6] Initializing WiFi...");
    g_telemetry.begin(WiFiMode::AP_MODE);
    Serial.println("  WiFi OK");
    
    // Setup task parameters
    g_taskParams.imu = &g_imu;
    g_taskParams.gps = &g_gps;
    g_taskParams.alertManager = &g_alertManager;
    g_taskParams.logger = &g_logger;
    g_taskParams.telemetry = &g_telemetry;
    g_taskParams.state = &g_systemState;
    g_taskParams.imuBuffer = &g_imuBuffer;
    g_taskParams.gpsBuffer = &g_gpsBuffer;
    g_taskParams.logBuffer = &g_logBuffer;
    
    // Wait for GPS fix before starting
    Serial.println("\nWaiting for GPS fix...");
    if (g_gps.waitForFix(10000)) {  // 10 second timeout
        Serial.println("GPS fix acquired!");
        g_systemState.postEvent(SystemEvent::GPS_FIX);
    } else {
        Serial.println("GPS fix timeout - continuing without fix");
    }
    
    // Create RTOS tasks
    Serial.println("\nCreating tasks...");
    
    // Sensor task - Core 0, highest priority
    xTaskCreatePinnedToCore(
        sensorTask,
        "Sensor",
        STACK_SIZE_SENSOR,
        &g_taskParams,
        TASK_PRIORITY_SENSOR,
        &hSensorTask,
        CORE_SENSOR
    );
    Serial.println("  Sensor task created (Core 0, Prio " + String(TASK_PRIORITY_SENSOR) + ")");
    
    // Compute task - Core 0, high priority
    xTaskCreatePinnedToCore(
        computeTask,
        "Compute",
        STACK_SIZE_COMPUTE,
        &g_taskParams,
        TASK_PRIORITY_ALERT,
        &hComputeTask,
        CORE_COMPUTE
    );
    Serial.println("  Compute task created (Core 0, Prio " + String(TASK_PRIORITY_ALERT) + ")");
    
    // Logging task - Core 1, medium priority
    xTaskCreatePinnedToCore(
        loggingTask,
        "Logging",
        STACK_SIZE_LOGGING,
        &g_taskParams,
        TASK_PRIORITY_LOGGING,
        &hLoggingTask,
        CORE_LOGGING
    );
    Serial.println("  Logging task created (Core 1, Prio " + String(TASK_PRIORITY_LOGGING) + ")");
    
    // Telemetry task - Core 1, lower priority
    xTaskCreatePinnedToCore(
        telemetryTask,
        "Telemetry",
        STACK_SIZE_TELEMETRY,
        &g_taskParams,
        TASK_PRIORITY_TELEMETRY,
        &hTelemetryTask,
        CORE_TELEMETRY
    );
    Serial.println("  Telemetry task created (Core 1, Prio " + String(TASK_PRIORITY_TELEMETRY) + ")");
    
    // Alert task - Core 0
    xTaskCreatePinnedToCore(
        alertTask,
        "Alert",
        STACK_SIZE_ALERT,
        &g_taskParams,
        TASK_PRIORITY_ALERT,
        &hAlertTask,
        CORE_SENSOR
    );
    Serial.println("  Alert task created");
    
    // Status task - Core 1, lowest priority
    xTaskCreatePinnedToCore(
        statusTask,
        "Status",
        STACK_SIZE_STATUS,
        &g_taskParams,
        TASK_PRIORITY_STATUS,
        &hStatusTask,
        CORE_STATUS
    );
    Serial.println("  Status task created");
    
    // Transition to ready state
    g_systemState.transitionTo(SystemState::READY, SystemEvent::SENSOR_READY);
    
    // Auto-start recording
    delay(1000);
    g_systemState.transitionTo(SystemState::RECORDING, SystemEvent::BUTTON_PRESS);
    
    Serial.println("\n========================================");
    Serial.println("System Ready - Recording Started");
    Serial.println("WiFi AP: " + String(WIFI_AP_SSID));
    Serial.println("IP: " + g_telemetry.getLocalIP().toString());
    Serial.println("========================================");
    
    // Print memory info
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
}

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

void handleSerialCommand(char cmd);

// =============================================================================
// MAIN LOOP (minimal - everything runs in tasks)
// =============================================================================

void loop() {
    // Process system events
    g_systemState.processEvents();
    
    // Handle serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        handleSerialCommand(cmd);
    }
    
    // Idle - let tasks run
    vTaskDelay(pdMS_TO_TICKS(100));
}

// =============================================================================
// COMMAND HANDLER
// =============================================================================

void handleSerialCommand(char cmd) {
    switch (cmd) {
        case 'r':  // Start recording
            g_systemState.transitionTo(SystemState::RECORDING, SystemEvent::BUTTON_PRESS);
            Serial.println("Recording started");
            break;
            
        case 's':  // Stop recording
            g_systemState.transitionTo(SystemState::READY, SystemEvent::BUTTON_PRESS);
            Serial.println("Recording stopped");
            break;
            
        case 'f':  // Flush SD card
            g_logger.flush();
            Serial.println("SD card flushed");
            break;
            
        case 'c':  // Calibrate IMU
            Serial.println("Calibrating... keep still");
            g_imu.performCalibration(300);
            Serial.println("Calibration complete");
            break;
            
        case 't':  // Print task stats
            printTaskStats("Sensor", g_sensorStats);
            printTaskStats("Compute", g_computeStats);
            printTaskStats("Logging", g_loggingStats);
            break;
            
        case 'g':  // GPS status
            g_gps.printStatus();
            break;
            
        case 'a':  // Alert status
            g_alertManager.printStatus();
            break;
            
        case 'h':  // Help
            Serial.println("Commands:");
            Serial.println("  r - Start recording");
            Serial.println("  s - Stop recording");
            Serial.println("  f - Flush SD card");
            Serial.println("  c - Calibrate IMU");
            Serial.println("  t - Task statistics");
            Serial.println("  g - GPS status");
            Serial.println("  a - Alert status");
            Serial.println("  h - Help");
            break;
            
        default:
            break;
    }
}
