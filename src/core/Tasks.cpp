#include "Tasks.h"

// Task handles
TaskHandle_t hSensorTask = nullptr;
TaskHandle_t hComputeTask = nullptr;
TaskHandle_t hLoggingTask = nullptr;
TaskHandle_t hTelemetryTask = nullptr;
TaskHandle_t hAlertTask = nullptr;
TaskHandle_t hStatusTask = nullptr;

// Task statistics
TaskStats g_sensorStats = {0};
TaskStats g_computeStats = {0};
TaskStats g_loggingStats = {0};

// Utility functions
void updateTaskStats(TaskStats& stats, uint32_t duration) {
    stats.iterations++;
    stats.lastRunTime = millis();
    
    if (stats.minDuration == 0 || duration < stats.minDuration) {
        stats.minDuration = duration;
    }
    if (duration > stats.maxDuration) {
        stats.maxDuration = duration;
    }
    
    // Running average
    stats.avgDuration = (stats.avgDuration * (stats.iterations - 1) + duration) / stats.iterations;
}

void printTaskStats(const char* name, const TaskStats& stats) {
    DEBUG_PRINTF(4, "Task %s: iter=%lu, min=%lu, max=%lu, avg=%lu, misses=%lu\n",
                 name, stats.iterations, stats.minDuration, 
                 stats.maxDuration, stats.avgDuration, stats.deadlineMisses);
}

// =============================================================================
// SENSOR TASK - Highest Priority
// Runs on Core 0, reads IMU at 100Hz and GPS at 10Hz
// =============================================================================
void sensorTask(void* pvParameters) {
    TaskParameters* params = (TaskParameters*)pvParameters;
    IMU* imu = params->imu;
    GPS* gps = params->gps;
    RingBuffer<IMUData, IMU_BUFFER_SIZE>* imuBuffer = params->imuBuffer;
    RingBuffer<GPSData, GPS_BUFFER_SIZE>* gpsBuffer = params->gpsBuffer;
    
    IMUData imuData;
    GPSData gpsData;
    
    TickType_t lastIMUTime = xTaskGetTickCount();
    TickType_t lastGPSTime = xTaskGetTickCount();
    
    DEBUG_PRINTLN(3, "Sensor task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        uint32_t startTime = micros();
        
        // IMU sampling at 100Hz
        if (xTaskGetTickCount() - lastIMUTime >= IMU_INTERVAL_MS) {
            if (imu->read()) {
                imu->fillData(imuData, millis());
                if (!imuBuffer->push(imuData, 0)) {  // Non-blocking
                    // Buffer full - sensor data dropped
                    DEBUG_PRINTLN(4, "IMU buffer full!");
                }
            }
            lastIMUTime = xTaskGetTickCount();
        }
        
        // GPS update at 10Hz (process all available data)
        gps->update();
        
        if (xTaskGetTickCount() - lastGPSTime >= GPS_INTERVAL_MS) {
            gps->fillData(gpsData, millis());
            if (!gpsBuffer->push(gpsData, 0)) {
                DEBUG_PRINTLN(4, "GPS buffer full!");
            }
            lastGPSTime = xTaskGetTickCount();
        }
        
        // Stats
        updateTaskStats(g_sensorStats, micros() - startTime);
        
        // Yield to let other tasks run (but keep high priority)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// =============================================================================
// COMPUTE TASK - Data Processing
// Runs on Core 0, processes sensor fusion and alert detection
// =============================================================================
void computeTask(void* pvParameters) {
    TaskParameters* params = (TaskParameters*)pvParameters;
    AlertManager* alerts = params->alertManager;
    RingBuffer<IMUData, IMU_BUFFER_SIZE>* imuBuffer = params->imuBuffer;
    RingBuffer<GPSData, GPS_BUFFER_SIZE>* gpsBuffer = params->gpsBuffer;
    RingBuffer<TelemetryPacket, LOG_BUFFER_SIZE>* logBuffer = params->logBuffer;
    SystemStateManager* state = params->state;
    
    IMUData imuData;
    GPSData gpsData;
    TelemetryPacket packet;
    
    IMUData latestIMU = {0};
    GPSData latestGPS = {0};
    
    uint16_t sequence = 0;
    
    DEBUG_PRINTLN(3, "Compute task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        uint32_t startTime = micros();
        
        // Process all available IMU data
        while (imuBuffer->pop(imuData, 0)) {
            latestIMU = imuData;
        }
        
        // Process all available GPS data
        while (gpsBuffer->pop(gpsData, 0)) {
            latestGPS = gpsData;
        }
        
        // Run alert detection
        alerts->process(latestIMU, latestGPS);
        
        // Build telemetry packet if system is recording
        if (state->isRecording()) {
            packet.magic = PACKET_MAGIC;
            packet.version = PACKET_VERSION;
            packet.sequence = sequence++;
            packet.timestamp_ms = millis();
            packet.imu = latestIMU;
            packet.gps = latestGPS;
            packet.crc16 = 0;  // TODO: Calculate CRC
            
            // Push to logging buffer
            if (!logBuffer->push(packet, 0)) {
                DEBUG_PRINTLN(4, "Log buffer full!");
            }
        }
        
        // Stats
        updateTaskStats(g_computeStats, micros() - startTime);
        
        // Run at 50Hz
        vTaskDelay(LOG_INTERVAL_MS);
    }
}

// =============================================================================
// LOGGING TASK - SD Card Operations
// Runs on Core 1, handles blocking SD card writes
// =============================================================================
void loggingTask(void* pvParameters) {
    TaskParameters* params = (TaskParameters*)pvParameters;
    BinaryLogger* logger = params->logger;
    RingBuffer<TelemetryPacket, LOG_BUFFER_SIZE>* logBuffer = params->logBuffer;
    SystemStateManager* state = params->state;
    
    TelemetryPacket packet;
    TickType_t lastFlushTime = xTaskGetTickCount();
    int writeCount = 0;
    
    DEBUG_PRINTLN(3, "Logging task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        uint32_t startTime = micros();
        
        // Process all available packets
        bool hadData = false;
        while (logBuffer->pop(packet, 0)) {
            if (state->isRecording()) {
                if (logger->write(packet)) {
                    hadData = true;
                    writeCount++;
                }
            }
        }
        
        // Periodic flush (every 5 seconds or 100 writes)
        if (writeCount >= FLUSH_INTERVAL_WRITES ||
            xTaskGetTickCount() - lastFlushTime >= pdMS_TO_TICKS(FLUSH_INTERVAL_MS)) {
            
            if (writeCount > 0) {
                logger->flush();
                writeCount = 0;
                lastFlushTime = xTaskGetTickCount();
            }
        }
        
        // Stats
        if (hadData) {
            updateTaskStats(g_loggingStats, micros() - startTime);
        }
        
        // Lower priority task - run less frequently
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =============================================================================
// TELEMETRY TASK - WiFi Streaming
// Runs on Core 1, handles network operations
// =============================================================================
void telemetryTask(void* pvParameters) {
    TaskParameters* params = (TaskParameters*)pvParameters;
    WiFiTelemetry* telemetry = params->telemetry;
    RingBuffer<TelemetryPacket, TELEMETRY_BUFFER_SIZE>* logBuffer = params->logBuffer;
    SystemStateManager* state = params->state;
    
    TelemetryPacket packet;
    TelemetryPacket lastPacket = {0};
    
    DEBUG_PRINTLN(3, "Telemetry task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        // Process web clients
        telemetry->handleWebClient();
        
        // Stream data if connected and recording
        if (telemetry->isConnected() && state->isRecording()) {
            // Get latest packet (peek at buffer without removing)
            // For streaming, we don't need every packet - just latest
            
            // Simple approach: sample directly at telemetry rate
            lastPacket.magic = PACKET_MAGIC;
            lastPacket.version = PACKET_VERSION;
            lastPacket.timestamp_ms = millis();
            // TODO: Fill with actual data from shared state
            
            telemetry->stream(lastPacket);
        }
        
        // Run at telemetry rate
        vTaskDelay(TELEMETRY_INTERVAL_MS);
    }
}

// =============================================================================
// ALERT TASK - Alert Handling
// Processes alert queue and triggers notifications
// =============================================================================
void alertTask(void* pvParameters) {
    TaskParameters* params = (TaskParameters*)pvParameters;
    AlertManager* alerts = params->alertManager;
    SystemStateManager* state = params->state;
    
    AlertEvent alert;
    
    DEBUG_PRINTLN(3, "Alert task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        // Process alerts from queue
        while (alerts->getAlert(alert, 0)) {
            // Handle alert based on severity
            switch (alert.severity) {
                case AlertSeverity::CRITICAL:
                    // Critical alerts could trigger recording stop
                    DEBUG_PRINTF(1, "CRITICAL ALERT: %d, value=%.2f\n",
                                 static_cast<int>(alert.type), alert.value);
                    break;
                    
                case AlertSeverity::WARNING:
                    DEBUG_PRINTF(2, "WARNING: %d, value=%.2f\n",
                                 static_cast<int>(alert.type), alert.value);
                    break;
                    
                case AlertSeverity::INFO:
                default:
                    DEBUG_PRINTF(4, "INFO: %d, value=%.2f\n",
                                 static_cast<int>(alert.type), alert.value);
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// =============================================================================
// STATUS TASK - LED and System Monitoring
// Updates status LED and prints diagnostics
// =============================================================================
void statusTask(void* pvParameters) {
    TaskParameters* params = (TaskParameters*)pvParameters;
    SystemStateManager* state = params->state;
    BinaryLogger* logger = params->logger;
    
    // LED patterns
    const int PATTERN_READY[] = {100, 900, -1};      // Slow blink
    const int PATTERN_RECORDING[] = {50, 50, 50, 850, -1};  // Triple blink
    const int PATTERN_ERROR[] = {100, 100, -1};      // Fast blink
    const int PATTERN_INIT[] = {500, 500, -1};       // Medium blink
    
    int patternIndex = 0;
    int patternStep = 0;
    bool ledState = false;
    TickType_t lastToggle = xTaskGetTickCount();
    
    DEBUG_PRINTLN(3, "Status task started on Core " + String(xPortGetCoreID()));
    
    // Setup LED pins
    pinMode(LED_PIN_RED, OUTPUT);
    pinMode(LED_PIN_GREEN, OUTPUT);
    pinMode(LED_PIN_BLUE, OUTPUT);
    
    while (true) {
        SystemState currentState = state->getState();
        const int* pattern = PATTERN_INIT;
        
        // Select pattern based on state
        switch (currentState) {
            case SystemState::INITIALIZING:
            case SystemState::CALIBRATING:
                pattern = PATTERN_INIT;
                digitalWrite(LED_PIN_RED, HIGH);
                digitalWrite(LED_PIN_GREEN, LOW);
                break;
                
            case SystemState::READY:
                pattern = PATTERN_READY;
                digitalWrite(LED_PIN_RED, LOW);
                digitalWrite(LED_PIN_GREEN, HIGH);
                break;
                
            case SystemState::RECORDING:
                pattern = PATTERN_RECORDING;
                digitalWrite(LED_PIN_RED, LOW);
                digitalWrite(LED_PIN_GREEN, HIGH);
                break;
                
            case SystemState::ERROR:
                pattern = PATTERN_ERROR;
                digitalWrite(LED_PIN_RED, HIGH);
                digitalWrite(LED_PIN_GREEN, LOW);
                break;
                
            default:
                break;
        }
        
        // Process pattern
        int delayMs = pattern[patternStep];
        if (delayMs < 0) {
            patternStep = 0;
            delayMs = pattern[0];
        }
        
        if (xTaskGetTickCount() - lastToggle >= pdMS_TO_TICKS(delayMs)) {
            ledState = !ledState;
            digitalWrite(LED_PIN_BLUE, ledState ? HIGH : LOW);
            patternStep++;
            lastToggle = xTaskGetTickCount();
        }
        
        // Print stats every 10 seconds
        static TickType_t lastStatsTime = 0;
        if (xTaskGetTickCount() - lastStatsTime >= pdMS_TO_TICKS(10000)) {
            LogStats logStats = logger->getStats();
            DEBUG_PRINTF(3, "Stats: LOG=%lu pkts, drops=%lu, SD=%luKB\n",
                         logStats.packetsWritten, logStats.drops,
                         logStats.bytesWritten / 1024);
            
            lastStatsTime = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
