#include "gps.h"
#include "storage.h"
#include "accelerometer.h"
#include "coreTaskSDWifi.h"

#define logDelayMs 100

coreTaskSDWifiHandleParams_t TaskSDWifiParams { "", 0 }

void setup() {
  // Serial Monitor
  Serial.begin(115200);

  // Initialize subsystems
  initGPS();
  initAccelerometer();
  initSD();

  xTaskCreatePinnedToCore(
    coreTaskSDWifiFunction, // Task code
    "coreTaskSDWifi", // Task name
    2048, // Stack size
    &myNumber, // Params  
    1, // Priority
    coreTaskSDWifiHandle, // Task handle
    1 // Core 0
  );
}

unsigned long ms = 0;

void loop() {
  updateGPSData();
  updateAccelerometerData();
  getGForce();

  unsigned long currentMillis = millis();

  if (currentMillis - ms >= logDelayMs) {
    SDFileWriteln("live.csv", formatLogData(gps.time, gps.latitude, gps.longitude, getGForce(), gps.satCount));

    ms = currentMillis;
  }
}