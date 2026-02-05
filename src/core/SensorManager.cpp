#include "SensorManager.h"

SensorManager::SensorManager() {}

SensorManager::~SensorManager() {
    // Note: SensorManager does not own the sensors, main does.
    // Or we can decide it owns them. For simple Arduino, manual memory management in setup() is fine.
    sensors.clear();
}

void SensorManager::addSensor(Sensor* sensor) {
    sensors.push_back(sensor);
}

void SensorManager::begin() {
    for (Sensor* sensor : sensors) {
        if (!sensor->begin()) {
            Serial.println("Failed to initialize a sensor!");
        }
    }
}

void SensorManager::update() {
    for (Sensor* sensor : sensors) {
        sensor->update();
    }
}

void SensorManager::printCombinedHeader(Print& p) {
    p.print("Timestamp,"); 
    for (size_t i = 0; i < sensors.size(); i++) {
        sensors[i]->printHeaderCSV(p);
        if (i < sensors.size() - 1) {
            p.print(",");
        }
    }
}

void SensorManager::printCombinedData(Print& p) {
    p.print(millis());
    p.print(","); 
    for (size_t i = 0; i < sensors.size(); i++) {
        sensors[i]->printDataCSV(p);
        if (i < sensors.size() - 1) {
             p.print(",");
        }
    }
}
