#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "Sensor.h"
#include <vector>

class SensorManager {
private:
    std::vector<Sensor*> sensors;

public:
    SensorManager();
    ~SensorManager();

    void addSensor(Sensor* sensor);
    void begin();
    void update();
    void printCombinedHeader(Print& p);
    void printCombinedData(Print& p);
};

#endif // SENSOR_MANAGER_H
