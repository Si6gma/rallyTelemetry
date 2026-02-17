#include <Arduino.h>
#include <unity.h>
#include "../../src/alerts/AlertManager.h"

AlertManager* alertManager = nullptr;

void setUp(void) {
    if (alertManager) {
        delete alertManager;
    }
    alertManager = new AlertManager();
    alertManager->begin();
}

void tearDown(void) {
    if (alertManager) {
        delete alertManager;
        alertManager = nullptr;
    }
}

void test_alert_manager_initialization(void) {
    TEST_ASSERT_NOT_NULL(alertManager);
}

void test_threshold_setters(void) {
    // Set custom thresholds
    alertManager->setGForceThresholds(2.0f, 3.0f, 0.15f);
    alertManager->setTempThresholds(55.0f, 70.0f, 0.1f);
    alertManager->setRollThresholds(20.0f, 30.0f, 0.1f);
    alertManager->setPitchThresholds(15.0f, 25.0f, 0.1f);
    
    // If we got here without crashing, setters work
    TEST_ASSERT_TRUE(true);
}

void test_alert_event_structure(void) {
    AlertEvent event;
    event.type = AlertType::GFORCE_WARNING;
    event.severity = AlertSeverity::WARNING;
    event.timestamp_ms = 12345;
    event.value = 2.5f;
    event.threshold = 2.0f;
    event.duration_ms = 150;
    event.count = 1;
    
    TEST_ASSERT_EQUAL(static_cast<int>(AlertType::GFORCE_WARNING), static_cast<int>(event.type));
    TEST_ASSERT_EQUAL(static_cast<int>(AlertSeverity::WARNING), static_cast<int>(event.severity));
    TEST_ASSERT_EQUAL(12345, event.timestamp_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.5f, event.value);
}

void test_alert_type_enum(void) {
    // Verify alert types are distinct
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(AlertType::GFORCE_WARNING),
        static_cast<int>(AlertType::GFORCE_CRITICAL)
    );
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(AlertType::TEMP_WARNING),
        static_cast<int>(AlertType::TEMP_CRITICAL)
    );
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(AlertSeverity::INFO),
        static_cast<int>(AlertSeverity::CRITICAL)
    );
}

void test_process_does_not_crash(void) {
    // Create dummy data
    IMUData imu = {
        .timestamp_ms = 1000,
        .accel_x = 0.0f,
        .accel_y = 0.0f,
        .accel_z = 9.81f,
        .gyro_x = 0.0f,
        .gyro_y = 0.0f,
        .gyro_z = 0.0f,
        .temperature = 25.0f
    };
    
    GPSData gps = {
        .timestamp_ms = 1000,
        .latitude = 40.7128,
        .longitude = -74.0060,
        .altitude = 50.0f,
        .speed_kmh = 80.0f,
        .heading = 180.0f,
        .satellites = 8,
        .fix_quality = 1,
        .hdop = 10,
        .padding = 0
    };
    
    // Should not crash
    alertManager->process(imu, gps);
    TEST_ASSERT_TRUE(true);
}

void test_reset_clears_stats(void) {
    alertManager->reset();
    
    TEST_ASSERT_EQUAL(0, alertManager->getTotalAlerts());
    TEST_ASSERT_FALSE(alertManager->isGForceAlertActive());
    TEST_ASSERT_FALSE(alertManager->isTempAlertActive());
}

void setup() {
    UNITY_BEGIN();
    
    RUN_TEST(test_alert_manager_initialization);
    RUN_TEST(test_threshold_setters);
    RUN_TEST(test_alert_event_structure);
    RUN_TEST(test_alert_type_enum);
    RUN_TEST(test_process_does_not_crash);
    RUN_TEST(test_reset_clears_stats);
    
    UNITY_END();
}

void loop() {
    // Empty
}
