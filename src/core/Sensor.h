#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class Sensor {
public:
    virtual ~Sensor() {}
    
    // Initialize the sensor. Returns true if successful.
    virtual bool begin() = 0;
    
    // Update sensor data. fast access, called in loop.
    virtual void update() = 0;
    
    // Get the CSV header for this sensor's data.
    virtual void printHeaderCSV(Print& p) = 0;
    
    // Get the current data formatted as CSV.
    virtual void printDataCSV(Print& p) = 0;
};

#endif // SENSOR_H
