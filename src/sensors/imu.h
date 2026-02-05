#ifndef IMU_H
#define IMU_H

#include "../core/Sensor.h"
#include "../core/config.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class IMU : public Sensor {
private:
    Adafruit_MPU6050 mpu;
    float ax, ay, az;
    float gx, gy, gz;
    float temp;

public:
    IMU();
    bool begin() override;
    void update() override;
    void printHeaderCSV(Print& p) override;
    void printDataCSV(Print& p) override;
    float getGForce();
};

#endif // IMU_H
