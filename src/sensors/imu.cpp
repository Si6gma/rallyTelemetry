#include "imu.h"

IMU::IMU() {
    ax = ay = az = 0;
    gx = gy = gz = 0;
    temp = 0;
}

bool IMU::begin() {
    // Try to initialize MPU6050
    // Try default address 0x68 first
    if (!mpu.begin()) {
        // Optional: Could try 0x69 if 0x68 fails, but typically user sets this.
        return false;
    }
    
    // Set ranges - could be moved to config or setter methods
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // Rally cars hit hard bumps!
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);      // Fast spins
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // Low pass filter
    
    return true;
}

void IMU::update() {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    
    gx = g.gyro.x;
    gy = g.gyro.y;
    gz = g.gyro.z;
    
    temp = t.temperature;
}

void IMU::printHeaderCSV(Print& p) {
    p.print("AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,TempC,GForce");
}

void IMU::printDataCSV(Print& p) {
    p.print(ax); p.print(",");
    p.print(ay); p.print(",");
    p.print(az); p.print(",");
    p.print(gx); p.print(",");
    p.print(gy); p.print(",");
    p.print(gz); p.print(",");
    p.print(temp); p.print(",");
    p.print(getGForce());
}

float IMU::getGForce() {
    // a.acceleration puts out m/s^2. divide by gravity to get Gs.
    float acx = ax / GRAVITY_CONSTANT;
    float acy = ay / GRAVITY_CONSTANT;
    float acz = az / GRAVITY_CONSTANT; 
    return sqrt(acx * acx + acy * acy + acz * acz);
}
