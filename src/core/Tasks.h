/**
 * FreeRTOS Task Definitions
 * 
 * Task implementations for the rally telemetry system.
 * Each task runs independently with its own priority and core assignment.
 */

#pragma once

#include "config.h"
#include "SystemState.h"
#include "../sensors/imu.h"
#include "../sensors/gps.h"
#include "../alerts/AlertManager.h"
#include "../storage/BinaryLogger.h"
#include "../telemetry/WiFiTelemetry.h"
#include "../utils/RingBuffer.h"

// Task function prototypes
void sensorTask(void* pvParameters);
void computeTask(void* pvParameters);
void loggingTask(void* pvParameters);
void telemetryTask(void* pvParameters);
void alertTask(void* pvParameters);
void statusTask(void* pvParameters);

// Task parameter structure
typedef struct {
    IMU* imu;
    GPS* gps;
    AlertManager* alertManager;
    BinaryLogger* logger;
    WiFiTelemetry* telemetry;
    SystemStateManager* state;
    
    // Data flow buffers
    RingBuffer<IMUData, IMU_BUFFER_SIZE>* imuBuffer;
    RingBuffer<GPSData, GPS_BUFFER_SIZE>* gpsBuffer;
    RingBuffer<TelemetryPacket, LOG_BUFFER_SIZE>* logBuffer;
} TaskParameters;

// Task handles (for external control)
extern TaskHandle_t hSensorTask;
extern TaskHandle_t hComputeTask;
extern TaskHandle_t hLoggingTask;
extern TaskHandle_t hTelemetryTask;
extern TaskHandle_t hAlertTask;
extern TaskHandle_t hStatusTask;

// Task statistics
struct TaskStats {
    uint32_t iterations;
    uint32_t minDuration;
    uint32_t maxDuration;
    uint32_t avgDuration;
    uint32_t lastRunTime;
    uint32_t deadlineMisses;
};

extern TaskStats g_sensorStats;
extern TaskStats g_computeStats;
extern TaskStats g_loggingStats;

// Utility functions
void updateTaskStats(TaskStats& stats, uint32_t duration);
void printTaskStats(const char* name, const TaskStats& stats);
