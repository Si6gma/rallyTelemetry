/**
 * Real-Time Alert Manager
 * 
 * Monitors telemetry data against thresholds and triggers alerts.
 * Supports hysteresis to prevent alert spam.
 */

#pragma once

#include "../core/config.h"
#include "../utils/RingBuffer.h"

// Alert severity levels
enum class AlertSeverity : uint8_t {
    INFO = 0,
    WARNING = 1,
    CRITICAL = 2
};

// Alert with full context
struct AlertEvent {
    AlertType type;
    AlertSeverity severity;
    uint32_t timestamp_ms;
    float value;
    float threshold;
    float duration_ms;
    uint8_t count;  // Number of consecutive triggers
};

// Threshold configuration
struct ThresholdConfig {
    float warning;
    float critical;
    float hysteresis;  // Percentage to clear alert (e.g., 0.1 = 10% below threshold)
    uint32_t minDurationMs;  // Minimum time above threshold to trigger
};

class AlertManager {
private:
    // Thresholds
    ThresholdConfig gForceThreshold;
    ThresholdConfig tempThreshold;
    ThresholdConfig rollThreshold;
    ThresholdConfig pitchThreshold;
    
    // State tracking for hysteresis
    struct AlertState {
        bool active;
        uint32_t triggerTime;
        uint32_t lastAlertTime;
        uint8_t consecutiveCount;
        float maxValue;
    };
    
    AlertState gForceState;
    AlertState tempState;
    AlertState rollState;
    AlertState pitchState;
    AlertState gpsState;
    
    // Alert queue for async processing
    QueueHandle_t alertQueue = nullptr;
    
    // Alert callback
    using AlertCallback = void (*)(const AlertEvent&);
    AlertCallback callback = nullptr;
    
    // Alert history (circular buffer)
    static const size_t HISTORY_SIZE = 32;
    AlertEvent alertHistory[HISTORY_SIZE];
    size_t historyIndex = 0;
    size_t historyCount = 0;
    SemaphoreHandle_t historyMutex = nullptr;
    
    // Statistics
    uint32_t totalAlerts = 0;
    uint32_t alertsByType[12] = {0};
    
    bool checkThreshold(float value, const ThresholdConfig& config, AlertState& state, 
                        uint32_t now, AlertType warningType, AlertType criticalType,
                        AlertEvent& outEvent);
    void recordAlert(const AlertEvent& event);
    const char* alertTypeToString(AlertType type) const;
    const char* severityToString(AlertSeverity severity) const;
    
public:
    AlertManager();
    ~AlertManager();
    
    bool begin();
    void end();
    
    // Configuration
    void setGForceThresholds(float warn, float crit, float hysteresis = 0.1f);
    void setTempThresholds(float warn, float crit, float hysteresis = 0.1f);
    void setRollThresholds(float warn, float crit, float hysteresis = 0.1f);
    void setPitchThresholds(float warn, float crit, float hysteresis = 0.1f);
    
    // Register callback for immediate alert notification
    void setCallback(AlertCallback cb);
    
    // Main monitoring function - call from compute task
    void process(const IMUData& imu, const GPSData& gps);
    
    // Queue access (for task communication)
    bool getAlert(AlertEvent& event, TickType_t timeout = 0);
    
    // History access
    size_t getHistory(AlertEvent* buffer, size_t maxCount);
    void clearHistory();
    
    // Statistics
    uint32_t getTotalAlerts() const { return totalAlerts; }
    uint32_t getAlertCount(AlertType type) const;
    
    // Current state queries
    bool isGForceAlertActive() const { return gForceState.active; }
    bool isTempAlertActive() const { return tempState.active; }
    float getCurrentGForceMax() const { return gForceState.maxValue; }
    
    // Reset
    void reset();
    
    // Debug
    void printStatus() const;
};
