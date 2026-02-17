#include <Arduino.h>
#include <unity.h>
#include "../../src/core/config.h"

void setUp(void) {
    // Empty
}

void tearDown(void) {
    // Empty
}

void test_data_structure_sizes(void) {
    // Verify packed structures have correct sizes
    TEST_ASSERT_EQUAL_MESSAGE(32, sizeof(IMUData), "IMUData size should be 32 bytes");
    TEST_ASSERT_EQUAL_MESSAGE(36, sizeof(GPSData), "GPSData size should be 36 bytes");
    TEST_ASSERT_EQUAL_MESSAGE(82, sizeof(TelemetryPacket), "TelemetryPacket size should be 82 bytes");
}

void test_packet_magic_constant(void) {
    TEST_ASSERT_EQUAL_UINT32(0x52414C4C, PACKET_MAGIC); // 'RALL'
}

void test_sampling_rates(void) {
    // Verify timing calculations
    TEST_ASSERT_EQUAL(100, IMU_SAMPLE_RATE_HZ);
    TEST_ASSERT_EQUAL(10, GPS_SAMPLE_RATE_HZ);
    TEST_ASSERT_EQUAL(50, LOG_RATE_HZ);
    
    // Verify intervals
    TEST_ASSERT_EQUAL(pdMS_TO_TICKS(10), IMU_INTERVAL_MS);   // 100Hz = 10ms
    TEST_ASSERT_EQUAL(pdMS_TO_TICKS(100), GPS_INTERVAL_MS);  // 10Hz = 100ms
    TEST_ASSERT_EQUAL(pdMS_TO_TICKS(20), LOG_INTERVAL_MS);   // 50Hz = 20ms
}

void test_alert_thresholds(void) {
    // Verify alert thresholds are reasonable
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.5f, ALERT_G_FORCE_WARN);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.5f, ALERT_G_FORCE_CRIT);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 25.0f, ALERT_ROLL_WARN);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 35.0f, ALERT_ROLL_CRIT);
}

void test_buffer_sizes_power_of_two(void) {
    // Ring buffer sizes must be power of 2 for efficient masking
    TEST_ASSERT_EQUAL(0, IMU_BUFFER_SIZE & (IMU_BUFFER_SIZE - 1));
    TEST_ASSERT_EQUAL(0, GPS_BUFFER_SIZE & (GPS_BUFFER_SIZE - 1));
    TEST_ASSERT_EQUAL(0, LOG_BUFFER_SIZE & (LOG_BUFFER_SIZE - 1));
}

void test_constants(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 9.80665f, GRAVITY_MS2);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 3.14159f, PI_F);
}

void setup() {
    UNITY_BEGIN();
    
    RUN_TEST(test_data_structure_sizes);
    RUN_TEST(test_packet_magic_constant);
    RUN_TEST(test_sampling_rates);
    RUN_TEST(test_alert_thresholds);
    RUN_TEST(test_buffer_sizes_power_of_two);
    RUN_TEST(test_constants);
    
    UNITY_END();
}

void loop() {
    // Empty
}
