#include "gps.h"
#include "storage.h"
#include "accelerometer.h"

#define logDelayMs 100

void setup() {
  // Serial Monitor
  Serial.begin(115200);

  initGPS();
  initAccelerometer();
  initSD();

  if (!SDFileExists("live.csv")) {
    SDFileWriteln("live.csv", logHeaderData());
  }
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