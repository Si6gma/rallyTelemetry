#include <Arduino.h>
#include <unity.h>
#include "../../src/sensors/imu.h"

void setUp(void) {
    // Empty
}

void tearDown(void) {
    // Empty
}

void test_gforce_calculation(void) {
    // Test G-force calculation with known values
    // 1G = 9.80665 m/s^2
    float ax = 9.80665f;  // 1G in X
    float ay = 0.0f;
    float az = 0.0f;
    
    float gforce = sqrt((ax/GRAVITY_MS2)*(ax/GRAVITY_MS2) + 
                        (ay/GRAVITY_MS2)*(ay/GRAVITY_MS2) + 
                        (az/GRAVITY_MS2)*(az/GRAVITY_MS2));
    
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0f, gforce);
    
    // Test 2G cornering
    ax = 19.6133f;  // 2G
    gforce = sqrt((ax/GRAVITY_MS2)*(ax/GRAVITY_MS2));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0f, gforce);
}

void test_roll_calculation(void) {
    // Level car should have 0 roll
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 9.80665f;
    
    float roll = atan2(ay, az) * RAD_TO_DEG;
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0f, roll);
    
    // 45 degree bank
    ay = 9.80665f;  // 1G lateral
    az = 9.80665f;  // 1G vertical
    roll = atan2(ay, az) * RAD_TO_DEG;
    TEST_ASSERT_FLOAT_WITHIN(0.1, 45.0f, roll);
}

void test_pitch_calculation(void) {
    // Level car should have 0 pitch
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 9.80665f;
    
    float pitch = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0f, pitch);
    
    // Climbing hill (nose up)
    ax = -4.903f;  // -0.5G
    az = 8.495f;   // cos(30) * 1G
    pitch = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;
    TEST_ASSERT_FLOAT_WITHIN(1.0, 30.0f, pitch);
}

void test_imu_data_structure(void) {
    IMUData data;
    data.timestamp_ms = 12345;
    data.accel_x = 1.0f;
    data.accel_y = 2.0f;
    data.accel_z = 3.0f;
    data.gyro_x = 4.0f;
    data.gyro_y = 5.0f;
    data.gyro_z = 6.0f;
    data.temperature = 25.0f;
    
    TEST_ASSERT_EQUAL(12345, data.timestamp_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0f, data.accel_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 6.0f, data.gyro_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 25.0f, data.temperature);
}

void setup() {
    UNITY_BEGIN();
    
    RUN_TEST(test_gforce_calculation);
    RUN_TEST(test_roll_calculation);
    RUN_TEST(test_pitch_calculation);
    RUN_TEST(test_imu_data_structure);
    
    UNITY_END();
}

void loop() {
    // Empty
}
