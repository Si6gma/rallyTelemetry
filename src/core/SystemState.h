/**
 * System State Manager
 * 
 * Thread-safe system state management with event notifications.
 * Handles transitions between initialization, recording, error, and shutdown states.
 */

#pragma once

#include "config.h"

// System events that can trigger state changes
enum class SystemEvent : uint8_t {
    NONE = 0,
    INIT_COMPLETE,
    SENSOR_READY,
    GPS_FIX,
    SD_READY,
    BUTTON_PRESS,
    ERROR_STORAGE,
    ERROR_SENSOR,
    ERROR_GPS,
    LOW_BATTERY,
    SHUTDOWN_REQUEST
};

// State change callback
using StateCallback = void (*)(SystemState oldState, SystemState newState, SystemEvent event);

class SystemStateManager {
private:
    volatile SystemState currentState = SystemState::INITIALIZING;
    SystemState previousState = SystemState::INITIALIZING;
    
    SemaphoreHandle_t stateMutex = nullptr;
    QueueHandle_t eventQueue = nullptr;
    
    StateCallback callback = nullptr;
    
    uint32_t stateEntryTime = 0;
    uint32_t stateDurations[6] = {0};  // Accumulated time in each state
    
    const char* stateToString(SystemState state) const;
    bool canTransition(SystemState from, SystemState to) const;
    
public:
    SystemStateManager();
    ~SystemStateManager();
    
    bool begin();
    void end();
    
    // State management
    SystemState getState() const { return currentState; }
    SystemState getPreviousState() const { return previousState; }
    bool transitionTo(SystemState newState, SystemEvent reason = SystemEvent::NONE);
    
    // Event handling
    void postEvent(SystemEvent event);
    bool processEvents();
    
    // Callback registration
    void setCallback(StateCallback cb);
    
    // State queries
    bool isRecording() const { return currentState == SystemState::RECORDING; }
    bool isError() const { return currentState == SystemState::ERROR; }
    bool isReady() const { return currentState == SystemState::READY || 
                                  currentState == SystemState::RECORDING; }
    
    // Timing
    uint32_t getTimeInCurrentState() const;
    uint32_t getTotalTimeInState(SystemState state) const;
    
    // Debug
    void printStatus() const;
};

// Global instance
extern SystemStateManager g_systemState;
