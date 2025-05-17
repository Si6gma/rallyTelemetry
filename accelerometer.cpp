#include "accelerometer.h"

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
accelerometer_t accelerometerData;

float getGForce() {
    float gx = accelerometerData.x / g;
    float gy = accelerometerData.y / g;
    float gz = accelerometerData.z / g;

    return sqrt(gx * gx + gy * gy + gz * gz);
}

void initAccelerometer() {
  if(!accel.begin())
  {
    Serial.println("No valid sensor found");
    while(1);
  }  
}

void updateAccelerometerData() {
  sensors_event_t event; 
  accel.getEvent(&event);

  accelerometerData.x = event.acceleration.x;
  accelerometerData.y = event.acceleration.y;
  accelerometerData.z = event.acceleration.z;
}