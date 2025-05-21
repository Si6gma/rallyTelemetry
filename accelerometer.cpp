#include "accelerometer.h"

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
extern accelerometer_t accelerometerData;

float getGForce() {
  float gx = accelerometerData.x / g;
  float gy = accelerometerData.y / g;
  return sqrt(gx * gx + gy * gy);
}

void initAccelerometer() {
  if (!accel.begin()) {
    Serial.println("No valid sensor found");
    while (1)
      ;
  }
}

void updateAccelerometerData() {
  sensors_event_t event;
  accel.getEvent(&event);

  accelerometerData.x = event.acceleration.x;
  accelerometerData.y = event.acceleration.y;
}