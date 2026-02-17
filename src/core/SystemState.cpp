#include "SystemState.h"

SystemStateManager::SystemStateManager() {
    stateMutex = xSemaphoreCreateMutex();
    eventQueue = xQueueCreate(16, sizeof(SystemEvent));
}

SystemStateManager::~SystemStateManager() {
    end();
}

bool SystemStateManager::begin() {
    currentState = SystemState::INITIALIZING;
    previousState = SystemState::INITIALIZING;
    stateEntryTime = millis();
    
    DEBUG_PRINTLN(3, "SystemStateManager initialized");
    return true;
}

void SystemStateManager::end() {
    if (eventQueue) {
        vQueueDelete(eventQueue);
        eventQueue = nullptr;
    }
    if (stateMutex) {
        vSemaphoreDelete(stateMutex);
        stateMutex = nullptr;
    }
}

bool SystemStateManager::canTransition(SystemState from, SystemState to) const {
    // Define valid state transitions
    switch (from) {
        case SystemState::INITIALIZING:
            return to == SystemState::CALIBRATING || 
                   to == SystemState::ERROR || 
                   to == SystemState::SHUTDOWN;
        case SystemState::CALIBRATING:
            return to == SystemState::READY || 
                   to == SystemState::ERROR || 
                   to == SystemState::SHUTDOWN;
        case SystemState::READY:
            return to == SystemState::RECORDING || 
                   to == SystemState::ERROR || 
                   to == SystemState::SHUTDOWN;
        case SystemState::RECORDING:
            return to == SystemState::READY || 
                   to == SystemState::ERROR || 
                   to == SystemState::SHUTDOWN;
        case SystemState::ERROR:
            return to == SystemState::INITIALIZING || 
                   to == SystemState::SHUTDOWN;
        case SystemState::SHUTDOWN:
            return false;  // Terminal state
        default:
            return false;
    }
}

bool SystemStateManager::transitionTo(SystemState newState, SystemEvent reason) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    
    if (!canTransition(currentState, newState)) {
        DEBUG_PRINTF(2, "Invalid state transition: %s -> %s\n", 
                     stateToString(currentState), stateToString(newState));
        xSemaphoreGive(stateMutex);
        return false;
    }
    
    // Calculate time spent in previous state
    uint32_t now = millis();
    uint32_t duration = now - stateEntryTime;
    stateDurations[static_cast<int>(currentState)] += duration;
    
    // Perform transition
    previousState = currentState;
    currentState = newState;
    stateEntryTime = now;
    
    DEBUG_PRINTF(3, "State transition: %s -> %s (reason: %d)\n",
                 stateToString(previousState), stateToString(currentState), 
                 static_cast<int>(reason));
    
    // Notify callback
    if (callback) {
        callback(previousState, currentState, reason);
    }
    
    xSemaphoreGive(stateMutex);
    return true;
}

void SystemStateManager::postEvent(SystemEvent event) {
    if (eventQueue) {
        xQueueSend(eventQueue, &event, 0);  // Non-blocking
    }
}

bool SystemStateManager::processEvents() {
    SystemEvent event;
    bool processed = false;
    
    while (xQueueReceive(eventQueue, &event, 0) == pdTRUE) {
        processed = true;
        
        // Handle events that trigger automatic state transitions
        switch (event) {
            case SystemEvent::INIT_COMPLETE:
                if (currentState == SystemState::INITIALIZING) {
                    transitionTo(SystemState::CALIBRATING, event);
                }
                break;
                
            case SystemEvent::SENSOR_READY:
            case SystemEvent::GPS_FIX:
                if (currentState == SystemState::CALIBRATING) {
                    transitionTo(SystemState::READY, event);
                }
                break;
                
            case SystemEvent::BUTTON_PRESS:
                if (currentState == SystemState::READY) {
                    transitionTo(SystemState::RECORDING, event);
                } else if (currentState == SystemState::RECORDING) {
                    transitionTo(SystemState::READY, event);
                }
                break;
                
            case SystemEvent::ERROR_STORAGE:
            case SystemEvent::ERROR_SENSOR:
            case SystemEvent::ERROR_GPS:
                transitionTo(SystemState::ERROR, event);
                break;
                
            case SystemEvent::SHUTDOWN_REQUEST:
                transitionTo(SystemState::SHUTDOWN, event);
                break;
                
            default:
                break;
        }
    }
    
    return processed;
}

void SystemStateManager::setCallback(StateCallback cb) {
    callback = cb;
}

uint32_t SystemStateManager::getTimeInCurrentState() const {
    return millis() - stateEntryTime;
}

uint32_t SystemStateManager::getTotalTimeInState(SystemState state) const {
    int idx = static_cast<int>(state);
    if (idx >= 0 && idx < 6) {
        return stateDurations[idx];
    }
    return 0;
}

const char* SystemStateManager::stateToString(SystemState state) const {
    switch (state) {
        case SystemState::INITIALIZING: return "INIT";
        case SystemState::CALIBRATING:  return "CAL";
        case SystemState::READY:        return "READY";
        case SystemState::RECORDING:    return "REC";
        case SystemState::ERROR:        return "ERROR";
        case SystemState::SHUTDOWN:     return "OFF";
        default: return "UNKNOWN";
    }
}

void SystemStateManager::printStatus() const {
    DEBUG_PRINTLN(3, "System State:");
    DEBUG_PRINTF(3, "  Current: %s (%lu ms)\n", 
                 stateToString(currentState), getTimeInCurrentState());
    DEBUG_PRINTF(3, "  Previous: %s\n", stateToString(previousState));
}
