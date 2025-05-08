#include "gps.h"

#define GPS_BUFFER_SIZE 256

static char gpsBuffer[GPS_BUFFER_SIZE];
static int bufferIndex = 0;

void initGPS() {
  gps.latitude = 0.0;
  gps.longitude = 0.0;
  gps.fixQuality = 0;
  gps.satCount = 0;
}

void recieveGPSData(gpsData * data, HardwareSerial * gpsSerial) {
  while (gpsSerial->available() > 0) {
    char c = gpsSerial.read();
    // Serial.print(c);

    if (c == '\n') {
      if (strstr(gpsBuffer, "$GNGGA") != NULL)
        parseGNGGASentence(gpsBuffer);

      gpsBuffer[0] = '\0';
      bufferIndex = 0;

    } else if (bufferIndex < GPS_BUFFER_SIZE - 1)
      gpsBuffer[bufferIndex++] = c;
  }
}

double convertToDecimalDegrees(double raw) {
  int deg = (int)(raw / 100);
  double min = raw - deg * 100;
  return deg + (min / 60.0);
}

void parseGNGGASentence(const char *sentence) {
  char buffer[128];
  strncpy(buffer, sentence, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';

  char *ptr = buffer;
  char *token;
  int field = 0;

  double latitude = 0.0, longitude = 0.0;
  int fixQuality = 0, satCount = 0;

  while ((token = strsep(&ptr, ",")) != NULL) {
    field++;

    switch (field) {
      case 3:  // Latitude
        latitude = convertToDecimalDegrees(atof(token));
        break;

      case 4:  // N/S
        if (*token == 'S')
          latitude *= -1;
        break;

      case 5:  // Longitude
        longitude = convertToDecimalDegrees(atof(token));
        break;

      case 6:  // E/W
        if (*token == 'W')
          longitude *= -1;
        break;

      case 7:  // Fix quality
        fixQuality = atoi(token);
        break;

      case 8:  // Satellite count
        satCount = atoi(token);
        break;

      default:
        break;
    }
  }

  gps.latitude = latitude;
  gps.longitude = longitude;
  gps.fixQuality = fixQuality;
  gps.satCount = satCount;
}

void displayGPSData() {
  printf("Latitude: %.6f\r\n", gps.latitude);
  printf("Longitude: %.6f\r\n", gps.longitude);
  printf("Fix Quality: %d | Satellites: %d\r\n", gps.fixQuality, gps.satCount);
}
