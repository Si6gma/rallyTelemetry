#include "AlertManager.h"

AlertManager::AlertManager() {
    // Set default thresholds from config
    gForceThreshold.warning = ALERT_G_FORCE_WARN;
    gForceThreshold.critical = ALERT_G_FORCE_CRIT;
    gForceThreshold.hysteresis = 0.1f;
    gForceThreshold.minDurationMs = 100;  // 100ms
    
    tempThreshold.warning = ALERT_TEMP_WARN;
    tempThreshold.critical = ALERT_TEMP_CRIT;
    tempThreshold.hysteresis = 0.05f;
    tempThreshold.minDurationMs = 1000;  // 1 second
    
    rollThreshold.warning = ALERT_ROLL_WARN;
    rollThreshold.critical = ALERT_ROLL_CRIT;
    rollThreshold.hysteresis = 0.1f;
    rollThreshold.minDurationMs = 200;
    
    pitchThreshold.warning = ALERT_PITCH_WARN;
    pitchThreshold.critical = ALERT_PITCH_CRIT;
    pitchThreshold.hysteresis = 0.1f;
    pitchThreshold.minDurationMs = 200;
    
    // Initialize states
    memset(&gForceState, 0, sizeof(gForceState));
    memset(&tempState, 0, sizeof(tempState));
    memset(&rollState, 0, sizeof(rollState));
    memset(&pitchState, 0, sizeof(pitchState));
    memset(&gpsState, 0, sizeof(gpsState));
}

AlertManager::~AlertManager() {
    end();
}

bool AlertManager::begin() {
    alertQueue = xQueueCreate(ALERT_QUEUE_SIZE, sizeof(AlertEvent));
    if (!alertQueue) return false;
    
    historyMutex = xSemaphoreCreateMutex();
    if (!historyMutex) {
        vQueueDelete(alertQueue);
        return false;
    }
    
    DEBUG_PRINTLN(3, "AlertManager initialized");
    return true;
}

void AlertManager::end() {
    if (alertQueue) {
        vQueueDelete(alertQueue);
        alertQueue = nullptr;
    }
    if (historyMutex) {
        vSemaphoreDelete(historyMutex);
        historyMutex = nullptr;
    }
}

void AlertManager::setGForceThresholds(float warn, float crit, float hysteresis) {
    gForceThreshold.warning = warn;
    gForceThreshold.critical = crit;
    gForceThreshold.hysteresis = hysteresis;
}

void AlertManager::setTempThresholds(float warn, float crit, float hysteresis) {
    tempThreshold.warning = warn;
    tempThreshold.critical = crit;
    tempThreshold.hysteresis = hysteresis;
}

void AlertManager::setRollThresholds(float warn, float crit, float hysteresis) {
    rollThreshold.warning = warn;
    rollThreshold.critical = crit;
    rollThreshold.hysteresis = hysteresis;
}

void AlertManager::setPitchThresholds(float warn, float crit, float hysteresis) {
    pitchThreshold.warning = warn;
    pitchThreshold.critical = crit;
    pitchThreshold.hysteresis = hysteresis;
}

void AlertManager::setCallback(AlertCallback cb) {
    callback = cb;
}

bool AlertManager::checkThreshold(float value, const ThresholdConfig& config, 
                                   AlertState& state, uint32_t now,
                                   AlertType warningType, AlertType criticalType,
                                   AlertEvent& outEvent) {
    bool triggered = false;
    
    // Check critical first
    if (value >= config.critical) {
        if (!state.active || state.consecutiveCount == 0) {
            state.triggerTime = now;
            state.maxValue = value;
        } else {
            state.maxValue = max(state.maxValue, value);
        }
        
        uint32_t duration = now - state.triggerTime;
        if (duration >= config.minDurationMs) {
            state.active = true;
            state.consecutiveCount++;
            
            outEvent.type = criticalType;
            outEvent.severity = AlertSeverity::CRITICAL;
            outEvent.timestamp_ms = now;
            outEvent.value = value;
            outEvent.threshold = config.critical;
            outEvent.duration_ms = duration;
            outEvent.count = state.consecutiveCount;
            triggered = true;
        }
    }
    // Then check warning
    else if (value >= config.warning) {
        if (!state.active || state.consecutiveCount == 0) {
            state.triggerTime = now;
            state.maxValue = value;
        } else {
            state.maxValue = max(state.maxValue, value);
        }
        
        uint32_t duration = now - state.triggerTime;
        // Only trigger warning if not already in critical
        if (duration >= config.minDurationMs && !state.active) {
            state.active = true;
            state.consecutiveCount++;
            
            outEvent.type = warningType;
            outEvent.severity = AlertSeverity::WARNING;
            outEvent.timestamp_ms = now;
            outEvent.value = value;
            outEvent.threshold = config.warning;
            outEvent.duration_ms = duration;
            outEvent.count = state.consecutiveCount;
            triggered = true;
        }
    }
    // Check if we should clear the alert (hysteresis)
    else {
        float clearThreshold = state.active ? 
            config.warning * (1.0f - config.hysteresis) : config.warning;
        
        if (value < clearThreshold) {
            state.active = false;
            state.consecutiveCount = 0;
            state.maxValue = 0;
        }
    }
    
    return triggered;
}

void AlertManager::process(const IMUData& imu, const GPSData& gps) {
    uint32_t now = millis();
    AlertEvent event;
    
    // Calculate G-force from accelerometer
    float ax_g = imu.accel_x / GRAVITY_MS2;
    float ay_g = imu.accel_y / GRAVITY_MS2;
    float az_g = imu.accel_z / GRAVITY_MS2;
    float gForce = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    
    // Check G-force
    if (checkThreshold(gForce, gForceThreshold, gForceState, now,
                       AlertType::GFORCE_WARNING, AlertType::GFORCE_CRITICAL, event)) {
        recordAlert(event);
    }
    
    // Check temperature
    if (checkThreshold(imu.temperature, tempThreshold, tempState, now,
                       AlertType::TEMP_WARNING, AlertType::TEMP_CRITICAL, event)) {
        recordAlert(event);
    }
    
    // Calculate roll and pitch from accelerometer
    float roll = atan2(imu.accel_y, imu.accel_z) * RAD_TO_DEG;
    float pitch = atan2(-imu.accel_x, sqrt(imu.accel_y * imu.accel_y + 
                                            imu.accel_z * imu.accel_z)) * RAD_TO_DEG;
    
    // Use absolute values for roll/pitch alerts
    float absRoll = fabs(roll);
    float absPitch = fabs(pitch);
    
    // Check roll
    if (checkThreshold(absRoll, rollThreshold, rollState, now,
                       AlertType::ROLL_WARNING, AlertType::ROLL_CRITICAL, event)) {
        recordAlert(event);
    }
    
    // Check pitch
    if (checkThreshold(absPitch, pitchThreshold, pitchState, now,
                       AlertType::PITCH_WARNING, AlertType::PITCH_CRITICAL, event)) {
        recordAlert(event);
    }
    
    // Check GPS fix status
    if (gps.fix_quality == 0) {
        if (!gpsState.active) {
            gpsState.active = true;
            gpsState.triggerTime = now;
            gpsState.consecutiveCount++;
            
            event.type = AlertType::GPS_LOST;
            event.severity = AlertSeverity::WARNING;
            event.timestamp_ms = now;
            event.value = 0;
            event.threshold = 1;
            event.duration_ms = 0;
            event.count = gpsState.consecutiveCount;
            recordAlert(event);
        }
    } else {
        gpsState.active = false;
        gpsState.consecutiveCount = 0;
    }
}

void AlertManager::recordAlert(const AlertEvent& event) {
    // Rate limiting - max 1 alert per type per second
    uint32_t now = millis();
    if (now - gForceState.lastAlertTime < 1000 && 
        (event.type == AlertType::GFORCE_WARNING || event.type == AlertType::GFORCE_CRITICAL)) {
        return;
    }
    
    // Update rate limit timestamp
    switch (event.type) {
        case AlertType::GFORCE_WARNING:
        case AlertType::GFORCE_CRITICAL:
            gForceState.lastAlertTime = now;
            break;
        default:
            break;
    }
    
    // Send to queue (non-blocking)
    xQueueSend(alertQueue, &event, 0);
    
    // Call immediate callback if registered
    if (callback) {
        callback(event);
    }
    
    // Record in history
    xSemaphoreTake(historyMutex, portMAX_DELAY);
    alertHistory[historyIndex] = event;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    if (historyCount < HISTORY_SIZE) historyCount++;
    xSemaphoreGive(historyMutex);
    
    // Update statistics
    totalAlerts++;
    size_t typeIndex = static_cast<size_t>(event.type);
    if (typeIndex < 12) {
        alertsByType[typeIndex]++;
    }
    
    // Log alert
    DEBUG_PRINTF(2, "[ALERT] %s %s: %.2f (threshold: %.2f, duration: %lums)\n",
                 severityToString(event.severity),
                 alertTypeToString(event.type),
                 event.value, event.threshold, event.duration_ms);
}

bool AlertManager::getAlert(AlertEvent& event, TickType_t timeout) {
    if (!alertQueue) return false;
    return xQueueReceive(alertQueue, &event, timeout) == pdTRUE;
}

size_t AlertManager::getHistory(AlertEvent* buffer, size_t maxCount) {
    if (!buffer || maxCount == 0) return 0;
    
    xSemaphoreTake(historyMutex, portMAX_DELAY);
    size_t count = min(maxCount, historyCount);
    
    for (size_t i = 0; i < count; i++) {
        size_t idx = (historyIndex + HISTORY_SIZE - count + i) % HISTORY_SIZE;
        buffer[i] = alertHistory[idx];
    }
    
    xSemaphoreGive(historyMutex);
    return count;
}

void AlertManager::clearHistory() {
    xSemaphoreTake(historyMutex, portMAX_DELAY);
    historyIndex = 0;
    historyCount = 0;
    xSemaphoreGive(historyMutex);
}

uint32_t AlertManager::getAlertCount(AlertType type) const {
    size_t idx = static_cast<size_t>(type);
    return (idx < 12) ? alertsByType[idx] : 0;
}

void AlertManager::reset() {
    totalAlerts = 0;
    memset(alertsByType, 0, sizeof(alertsByType));
    clearHistory();
    
    memset(&gForceState, 0, sizeof(gForceState));
    memset(&tempState, 0, sizeof(tempState));
    memset(&rollState, 0, sizeof(rollState));
    memset(&pitchState, 0, sizeof(pitchState));
    memset(&gpsState, 0, sizeof(gpsState));
}

const char* AlertManager::alertTypeToString(AlertType type) const {
    switch (type) {
        case AlertType::NONE: return "NONE";
        case AlertType::GFORCE_WARNING: return "GFORCE_WARNING";
        case AlertType::GFORCE_CRITICAL: return "GFORCE_CRITICAL";
        case AlertType::TEMP_WARNING: return "TEMP_WARNING";
        case AlertType::TEMP_CRITICAL: return "TEMP_CRITICAL";
        case AlertType::ROLL_WARNING: return "ROLL_WARNING";
        case AlertType::ROLL_CRITICAL: return "ROLL_CRITICAL";
        case AlertType::PITCH_WARNING: return "PITCH_WARNING";
        case AlertType::PITCH_CRITICAL: return "PITCH_CRITICAL";
        case AlertType::GPS_LOST: return "GPS_LOST";
        case AlertType::SD_ERROR: return "SD_ERROR";
        case AlertType::LOW_BATTERY: return "LOW_BATTERY";
        default: return "UNKNOWN";
    }
}

const char* AlertManager::severityToString(AlertSeverity severity) const {
    switch (severity) {
        case AlertSeverity::INFO: return "INFO";
        case AlertSeverity::WARNING: return "WARN";
        case AlertSeverity::CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}

void AlertManager::printStatus() const {
    DEBUG_PRINTLN(3, "AlertManager Status:");
    DEBUG_PRINTF(3, "  Total alerts: %lu\n", totalAlerts);
    DEBUG_PRINTF(3, "  G-Force alerts: %lu\n", getAlertCount(AlertType::GFORCE_WARNING) + 
                                                 getAlertCount(AlertType::GFORCE_CRITICAL));
    DEBUG_PRINTF(3, "  Active: G-Force=%s, Temp=%s\n",
                 gForceState.active ? "YES" : "NO",
                 tempState.active ? "YES" : "NO");
}
