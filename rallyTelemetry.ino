#include "gps.h"
#include "storage.h"
#include "accelerometer.h"

#define logDelayMs 100

void setup() {
  Serial.begin(115200);

  delay(1000);

  initGPS();
  initAccelerometer();
  initSD();

  delay(1000);

  if (!SDFileExists("/live.csv")) {
    Serial.println("Created live.csv");
    SDFileWriteln("/live.csv", logHeaderData());
  }
}

unsigned long ms = 0;

void loop() {
  updateGPSData();
  updateAccelerometerData();
  getGForce();

  unsigned long currentMillis = millis();

  if (currentMillis - ms >= logDelayMs) {
    SDFileWriteln("/live.csv", formatLogData(gps.time, gps.latitude, gps.longitude, getGForce(), gps.satCount));

    ms = currentMillis;
    Serial.println(gps.time);
  }
}