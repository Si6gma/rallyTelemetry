#include "gps.h"
#include "sdReader.h"
#include "accelerometer.h"

void setup(){
  // Serial Monitor
  Serial.begin(115200);

  initGPS();
  initAccelerometer();
  
  Serial.println("GPS started at 9600 baud rate, 10Hz");
}

void loop(){
  updateGPSData();
  updateAccelerometerData();

  displayGPSData();
  delay(1000);
  Serial.println("-------------------------------");
}