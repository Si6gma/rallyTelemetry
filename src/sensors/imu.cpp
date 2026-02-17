#include "imu.h"

// Static members
volatile bool IMU::dataReady = false;
SemaphoreHandle_t IMU::dataReadySemaphore = nullptr;

void IRAM_ATTR IMU::onDataReady() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    dataReady = true;
    if (dataReadySemaphore) {
        xSemaphoreGiveFromISR(dataReadySemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

IMU::IMU(TwoWire* i2c) : wire(i2c) {
    rawAx = rawAy = rawAz = 0;
    rawGx = rawGy = rawGz = 0;
    calAx = calAy = calAz = 0;
    calGx = calGy = calGz = 0;
    temperature = 0;
    roll = pitch = yaw = 0;
    gForce = 0;
    
    // Create semaphore for data ready notification
    if (!dataReadySemaphore) {
        dataReadySemaphore = xSemaphoreCreateBinary();
    }
}

bool IMU::begin() {
    if (!wire) wire = &Wire;
    
    // Initialize I2C
    wire->setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    wire->setClock(400000);  // 400kHz fast mode
    
    // Try to initialize MPU6050
    if (!mpu.begin(MPU6050_ADDR, wire)) {
        DEBUG_PRINTLN(1, "MPU6050 not found at address 0x68, trying 0x69");
        if (!mpu.begin(0x69, wire)) {
            DEBUG_PRINTLN(1, "MPU6050 initialization failed!");
            return false;
        }
    }
    
    // Configure for high-performance
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);     // Max range for rally impacts
    mpu.setGyroRange(MPU6050_RANGE_1000_DEG);          // High rotation rates
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);        // Balance latency vs noise
    
    // Enable data ready interrupt on pin (optional - GPIO 34)
    // mpu.setInterruptPinLatch(true);
    // mpu.setInterruptPinPolarity(true);
    // mpu.setInterrupt(MPU6050_INT_DATA_READY, true);
    // attachInterrupt(digitalPinToInterrupt(34), onDataReady, RISING);
    
    DEBUG_PRINTLN(3, "MPU6050 initialized successfully");
    DEBUG_PRINTLN(3, "  Accel range: +/- 16G");
    DEBUG_PRINTLN(3, "  Gyro range: +/- 1000 deg/s");
    DEBUG_PRINTLN(3, "  Filter: 44Hz");
    
    return true;
}

void IMU::end() {
    // Cleanup
}

bool IMU::waitForData(TickType_t timeout) {
    // Wait for data ready semaphore (interrupt-driven) or timeout
    if (dataReadySemaphore) {
        if (xSemaphoreTake(dataReadySemaphore, timeout) == pdTRUE) {
            return read();
        }
    }
    // Fallback to polling
    return read();
}

bool IMU::read() {
    sensors_event_t a, g, t;
    
    if (!mpu.getEvent(&a, &g, &t)) {
        errorCount++;
        return false;
    }
    
    // Store raw values
    rawAx = a.acceleration.x;
    rawAy = a.acceleration.y;
    rawAz = a.acceleration.z;
    rawGx = g.gyro.x;
    rawGy = g.gyro.y;
    rawGz = g.gyro.z;
    temperature = t.temperature;
    
    // Apply calibration
    calAx = (rawAx - calibration.accelBias[0]) * calibration.scale[0];
    calAy = (rawAy - calibration.accelBias[1]) * calibration.scale[1];
    calAz = (rawAz - calibration.accelBias[2]) * calibration.scale[2];
    
    calGx = rawGx - calibration.gyroBias[0];
    calGy = rawGy - calibration.gyroBias[1];
    calGz = rawGz - calibration.gyroBias[2];
    
    // In calibration mode, don't update computed values
    if (!calibrationMode) {
        computeOrientation();
    }
    
    sampleCount++;
    return true;
}

bool IMU::isDataReady() {
    // Check interrupt flag or poll status
    if (dataReady) {
        dataReady = false;
        return true;
    }
    return false;
}

void IMU::computeOrientation() {
    // Calculate roll and pitch from accelerometer
    // Roll = atan2(ay, az) * 180/pi
    // Pitch = atan2(-ax, sqrt(ay^2 + az^2)) * 180/pi
    
    roll = atan2(calAy, calAz) * RAD_TO_DEG;
    pitch = atan2(-calAx, sqrt(calAy * calAy + calAz * calAz)) * RAD_TO_DEG;
    
    // Yaw requires magnetometer (not available on MPU6050)
    // Integrate gyro Z for relative yaw (will drift)
    yaw += calGz * 0.01f;  // Assuming 100Hz sample rate
    
    // Normalize yaw to 0-360
    while (yaw < 0) yaw += 360;
    while (yaw >= 360) yaw -= 360;
    
    // Calculate total G-force
    float ax_g = calAx / GRAVITY_MS2;
    float ay_g = calAy / GRAVITY_MS2;
    float az_g = calAz / GRAVITY_MS2;
    gForce = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
}

void IMU::startCalibration() {
    calibrationMode = true;
    DEBUG_PRINTLN(3, "IMU calibration started - keep sensor still and level");
}

void IMU::stopCalibration() {
    calibrationMode = false;
}

bool IMU::performCalibration(uint32_t samples) {
    startCalibration();
    
    float accelSum[3] = {0, 0, 0};
    float gyroSum[3] = {0, 0, 0};
    uint32_t validSamples = 0;
    
    DEBUG_PRINTLN(3, "Collecting calibration samples...");
    
    for (uint32_t i = 0; i < samples; i++) {
        if (read()) {
            accelSum[0] += rawAx;
            accelSum[1] += rawAy;
            accelSum[2] += rawAz;
            gyroSum[0] += rawGx;
            gyroSum[1] += rawGy;
            gyroSum[2] += rawGz;
            validSamples++;
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz sampling during cal
    }
    
    if (validSamples < samples / 2) {
        DEBUG_PRINTLN(1, "Calibration failed - too many read errors");
        stopCalibration();
        return false;
    }
    
    // Calculate biases
    calibration.accelBias[0] = accelSum[0] / validSamples;
    calibration.accelBias[1] = accelSum[1] / validSamples;
    // Z should read 1G when level, so bias = avg - GRAVITY
    calibration.accelBias[2] = (accelSum[2] / validSamples) - GRAVITY_MS2;
    
    calibration.gyroBias[0] = gyroSum[0] / validSamples;
    calibration.gyroBias[1] = gyroSum[1] / validSamples;
    calibration.gyroBias[2] = gyroSum[2] / validSamples;
    
    calibration.scale[0] = calibration.scale[1] = calibration.scale[2] = 1.0f;
    calibration.isValid = true;
    
    DEBUG_PRINTLN(3, "Calibration complete:");
    DEBUG_PRINTF(3, "  Accel bias: X=%.3f Y=%.3f Z=%.3f\n", 
                 calibration.accelBias[0], calibration.accelBias[1], calibration.accelBias[2]);
    DEBUG_PRINTF(3, "  Gyro bias: X=%.3f Y=%.3f Z=%.3f\n",
                 calibration.gyroBias[0], calibration.gyroBias[1], calibration.gyroBias[2]);
    
    stopCalibration();
    return true;
}

void IMU::saveCalibration(const IMUCalibration& cal) {
    calibration = cal;
    // TODO: Save to NVS (non-volatile storage)
}

bool IMU::isHealthy() const {
    // Check for stuck values (sensor might be disconnected)
    if (sampleCount < 10) return true;  // Not enough samples yet
    
    // Check temperature is reasonable (-40 to +85 for MPU6050)
    if (temperature < -40.0f || temperature > 85.0f) return false;
    
    // Check accelerometer values are within range
    float maxAccel = 20.0f * GRAVITY_MS2;  // Slightly above 16G range
    if (fabs(calAx) > maxAccel || fabs(calAy) > maxAccel || fabs(calAz) > maxAccel) {
        return false;
    }
    
    return true;
}

void IMU::fillData(IMUData& data, uint32_t timestamp) {
    data.timestamp_ms = timestamp;
    data.accel_x = calAx;
    data.accel_y = calAy;
    data.accel_z = calAz;
    data.gyro_x = calGx;
    data.gyro_y = calGy;
    data.gyro_z = calGz;
    data.temperature = temperature;
}
