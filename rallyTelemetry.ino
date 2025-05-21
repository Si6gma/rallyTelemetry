#include "gps.h"
#include "storage.h"
#include "accelerometer.h"

void setup(){
  // Serial Monitor
  Serial.begin(115200);

  initGPS();
  initAccelerometer();
  initSD();
}

void loop(){
  updateGPSData();
  updateAccelerometerData();

  displayGPSData();
  delay(1000);
  Serial.println("-------------------------------");
}