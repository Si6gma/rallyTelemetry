#include "gps.h"

static char gpsBuffer[GPS_BUFFER_SIZE];
static int bufferIndex = 0;
HardwareSerial gpsSerial(2);

gpsData_t gps;

void parseGNGGASentence(const char *sentence);
double convertToDecimalDegrees(double raw);

void initGPS() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  gpsSerial.print("$PCAS02,100*1E\r\n");

  gps.latitude = 0.0;
  gps.longitude = 0.0;
  gps.fixQuality = 0;
  gps.satCount = 0;

  Serial.println("GPS started at 9600 baud rate, 10Hz");
}

void updateGPSData() {
  while (gpsSerial.available() > 0) {
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

  while ((token = strsep(&ptr, ",")) != NULL) {
    field++;

    switch (field) {
      case 2:  // UTC Time
        gps.time = atof(token);
        break;

      case 3:  // Latitude
        gps.latitude = convertToDecimalDegrees(atof(token));
        break;

      case 4:  // N/S
        if (*token == 'S')
          gps.latitude *= -1;
        break;

      case 5:  // Longitude
        gps.longitude = convertToDecimalDegrees(atof(token));
        break;

      case 6:  // E/W
        if (*token == 'W')
          gps.longitude *= -1;
        break;

      case 7:  // Fix quality
        gps.fixQuality = atoi(token);
        break;

      case 8:  // Satellite count
        gps.satCount = atoi(token);
        break;

      default:
        break;
    }
  }
}
