#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

#define g 9.80665;

typedef struct 
{
  float x;
  float y;
  float z;
} accelerometer_t;

float getGForce();
void updateAccelerometerData();
void initAccelerometer();

#endif