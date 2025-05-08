#ifndef GPS_H
#define GPS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <stdbool.h>

// GPS Data Structure
typedef struct
{
  double latitude;   // Current Latitude Value
  double longitude;  // Current Longitude Value
  int satCount;      // If Satellite Count is above 8, then good connection
  int fixQuality;    // If fix quality is 1, then GPS is connected
} gpsData;

// Function prototypes
void initGPS();
void recieveGPSData();
void displayGPSData();
void parseGNGGASentence(const char *);
double convertToDecimalDegrees(double);

#endif  // GPS_H
