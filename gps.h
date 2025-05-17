#ifndef GPS_H
#define GPS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <HardwareSerial.h>


#include <stdbool.h>

 // Define the RX and TX pins for Serial 2
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

#define GPS_BUFFER_SIZE 256

// GPS Data Structure
typedef struct
{
  double latitude;   // Current Latitude Value
  double longitude;  // Current Longitude Value
  int satCount;      // If Satellite Count is above 8, then good connection
  int fixQuality;    // If fix quality is 1, then GPS is connected
} gpsData_t;

// Function prototypes
void initGPS();
void updateGPSData();
void displayGPSData();
gpsData_t getGpsData();

#endif  // GPS_H
