/**
 * High-Performance IMU Sensor (MPU6050 with DMP)
 * 
 * Features:
 * - 100Hz sampling with hardware interrupt
 * - Digital Motion Processor (DMP) for sensor fusion
 * - 6-axis quaternion output for orientation
 * - Calibration and bias compensation
 */

#pragma once

#include "../core/config.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Calibration data structure
struct IMUCalibration {
    float accelBias[3] = {0, 0, 0};
    float gyroBias[3] = {0, 0, 0};
    float scale[3] = {1, 1, 1};
    bool isValid = false;
};

class IMU {
private:
    Adafruit_MPU6050 mpu;
    TwoWire* wire = nullptr;
    
    // Raw data
    float rawAx, rawAy, rawAz;
    float rawGx, rawGy, rawGz;
    float temperature;
    
    // Calibrated data
    float calAx, calAy, calAz;
    float calGx, calGy, calGz;
    
    // Orientation (calculated)
    float roll, pitch, yaw;
    float gForce;
    
    // Calibration
    IMUCalibration calibration;
    bool calibrationMode = false;
    
    // Statistics
    uint32_t sampleCount = 0;
    uint32_t errorCount = 0;
    
    // Hardware interrupt
    static void IRAM_ATTR onDataReady();
    static volatile bool dataReady;
    static SemaphoreHandle_t dataReadySemaphore;
    
    void computeOrientation();
    
public:
    IMU(TwoWire* i2c = &Wire);
    
    bool begin();
    void end();
    
    // Blocking read (for task-based operation)
    bool waitForData(TickType_t timeout = portMAX_DELAY);
    bool read();
    
    // Non-blocking check
    bool isDataReady();
    
    // Calibration
    void startCalibration();
    void stopCalibration();
    bool performCalibration(uint32_t sampleCount = 500);
    void saveCalibration(const IMUCalibration& cal);
    IMUCalibration getCalibration() const { return calibration; }
    
    // Data accessors
    float getAccelX() const { return calAx; }
    float getAccelY() const { return calAy; }
    float getAccelZ() const { return calAz; }
    float getGyroX() const { return calGx; }
    float getGyroY() const { return calGy; }
    float getGyroZ() const { return calGz; }
    float getTemperature() const { return temperature; }
    
    // Derived values
    float getRoll() const { return roll; }      // degrees
    float getPitch() const { return pitch; }    // degrees
    float getYaw() const { return yaw; }        // degrees (drifts without mag)
    float getGForce() const { return gForce; }
    
    // Vector accessors
    void getAccel(float& x, float& y, float& z) const { x = calAx; y = calAy; z = calAz; }
    void getGyro(float& x, float& y, float& z) const { x = calGx; y = calGy; z = calGz; }
    
    // Statistics
    uint32_t getSampleCount() const { return sampleCount; }
    uint32_t getErrorCount() const { return errorCount; }
    void resetStats() { sampleCount = errorCount = 0; }
    
    // Health check
    bool isHealthy() const;
    
    // Convert to packet format
    void fillData(IMUData& data, uint32_t timestamp);
};
