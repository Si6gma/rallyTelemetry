#include "gps.h"

GPS::GPS() : gpsSerial(2) {
    latitude = 0.0;
    longitude = 0.0;
    satCount = 0;
    fixQuality = 0;
    bufferIndex = 0;
}

bool GPS::begin() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(100);
    gpsSerial.print("$PCAS02,100*1E\r\n"); // 10Hz update rate
    return true;
}

void GPS::update() {
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();

        if (c == '\n') {
            gpsBuffer[bufferIndex] = '\0'; // Null terminate
            if (strstr(gpsBuffer, "$GNGGA") != NULL) {
                parseGNGGASentence(gpsBuffer);
            }
            bufferIndex = 0;
        } else if (bufferIndex < GPS_BUFFER_SIZE - 1) {
            gpsBuffer[bufferIndex++] = c;
        }
    }
}

void GPS::printHeaderCSV(Print& p) {
    p.print("GPS_Time,Latitude,Longitude,SatCount,FixQuality");
}

void GPS::printDataCSV(Print& p) {
    p.print(time);
    p.print(",");
    p.print(latitude, 6);
    p.print(",");
    p.print(longitude, 6);
    p.print(",");
    p.print(satCount);
    p.print(",");
    p.print(fixQuality);
}

double GPS::convertToDecimalDegrees(double raw) {
    int deg = (int)(raw / 100);
    double min = raw - deg * 100;
    return deg + (min / 60.0);
}

void GPS::parseGNGGASentence(const char *sentence) {
    char buffer[128];
    strncpy(buffer, sentence, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *ptr = buffer;
    char *token;
    int field = 0;

    while ((token = strsep(&ptr, ",")) != NULL) {
        field++;

        switch (field) {
            case 2: // UTC Time
                time = String(token);
                break;
            case 3: // Latitude
                latitude = convertToDecimalDegrees(atof(token));
                break;
            case 4: // N/S
                if (*token == 'S') latitude *= -1;
                break;
            case 5: // Longitude
                longitude = convertToDecimalDegrees(atof(token));
                break;
            case 6: // E/W
                if (*token == 'W') longitude *= -1;
                break;
            case 7: // Fix quality
                fixQuality = atoi(token);
                break;
            case 8: // Satellite count
                satCount = atoi(token);
                break;
        }
    }
}
