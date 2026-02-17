#include "gps.h"

GPS::GPS() : gpsSerial(2) {
    // UART2 on ESP32
}

bool GPS::begin(int baudRate) {
    // Start with default baud
    gpsSerial.begin(baudRate, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    delay(100);
    
    // Configure for 10Hz and GPS+GLONASS
    // UBX protocol commands for u-blox modules
    const uint8_t ubxCfgRate[] = {
        0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 
        0x64, 0x00,  // 100ms = 10Hz
        0x01, 0x00,  // navRate = 1
        0x01, 0x00,  // timeRef = GPS
        0x7A, 0x12   // checksum
    };
    
    // Send configuration
    gpsSerial.write(ubxCfgRate, sizeof(ubxCfgRate));
    delay(100);
    
    // Try NMEA command for generic modules
    gpsSerial.println("$PMTK220,100*2F");  // 10Hz update rate
    gpsSerial.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");  // GGA and RMC only
    
    configured = true;
    
    DEBUG_PRINTLN(3, "GPS initialized");
    DEBUG_PRINTF(3, "  Baud rate: %d\n", baudRate);
    
    return true;
}

void GPS::end() {
    gpsSerial.end();
}

void GPS::update() {
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        
        if (c == '$') {
            // Start of new sentence
            bufferIndex = 0;
            nmeaBuffer[bufferIndex++] = c;
        } else if (c == '\r' || c == '\n') {
            // End of sentence
            if (bufferIndex > 0 && bufferIndex < sizeof(nmeaBuffer)) {
                nmeaBuffer[bufferIndex] = '\0';
                if (validateChecksum(nmeaBuffer)) {
                    parseNMEA(nmeaBuffer);
                }
            }
            bufferIndex = 0;
        } else if (bufferIndex < sizeof(nmeaBuffer) - 1) {
            nmeaBuffer[bufferIndex++] = c;
        }
    }
}

bool GPS::parseNMEA(const char* sentence) {
    sentenceCount++;
    
    // Check sentence type
    if (strncmp(sentence, "$GNGGA", 6) == 0 || strncmp(sentence, "$GPGGA", 6) == 0) {
        return parseGGA(sentence + 7);
    } else if (strncmp(sentence, "$GNRMC", 6) == 0 || strncmp(sentence, "$GPRMC", 6) == 0) {
        return parseRMC(sentence + 7);
    } else if (strncmp(sentence, "$GNVTG", 6) == 0 || strncmp(sentence, "$GPVTG", 6) == 0) {
        return parseVTG(sentence + 7);
    } else if (strncmp(sentence, "$GNGSA", 6) == 0 || strncmp(sentence, "$GPGSA", 6) == 0) {
        return parseGSA(sentence + 7);
    }
    
    return false;
}

bool GPS::parseGGA(const char* data) {
    // $GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    char buffer[128];
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token;
    char* ptr = buffer;
    int field = 0;
    
    while ((token = strsep(&ptr, ",")) != nullptr) {
        switch (field) {
            case 0:  // Time (HHMMSS.SS)
                if (strlen(token) >= 6) {
                    hours = (token[0] - '0') * 10 + (token[1] - '0');
                    minutes = (token[2] - '0') * 10 + (token[3] - '0');
                    seconds = atof(token + 4);
                }
                break;
            case 1:  // Latitude
                latitude = parseCoordinate(token, nullptr);
                break;
            case 2:  // N/S
                if (token[0] == 'S') latitude = -latitude;
                break;
            case 3:  // Longitude
                longitude = parseCoordinate(token, nullptr);
                break;
            case 4:  // E/W
                if (token[0] == 'W') longitude = -longitude;
                break;
            case 5:  // Fix quality
                fixType = static_cast<GPSFixType>(atoi(token));
                break;
            case 6:  // Satellites
                satellites = atoi(token);
                break;
            case 7:  // HDOP
                hdop = atof(token);
                break;
            case 8:  // Altitude
                altitude = atof(token);
                break;
        }
        field++;
    }
    
    if (hasFix()) {
        lastFixTime = millis();
        validSentenceCount++;
    }
    
    return true;
}

bool GPS::parseRMC(const char* data) {
    // $GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
    char buffer[128];
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token;
    char* ptr = buffer;
    int field = 0;
    
    while ((token = strsep(&ptr, ",")) != nullptr) {
        switch (field) {
            case 0:  // Time
                if (strlen(token) >= 6) {
                    hours = (token[0] - '0') * 10 + (token[1] - '0');
                    minutes = (token[2] - '0') * 10 + (token[3] - '0');
                    seconds = atof(token + 4);
                }
                break;
            case 1:  // Status (A=valid, V=warning)
                navStatus = (token[0] == 'A') ? GPSNavStatus::VALID : GPSNavStatus::WARNING;
                break;
            case 2:  // Latitude
                latitude = parseCoordinate(token, nullptr);
                break;
            case 3:  // N/S
                if (token[0] == 'S') latitude = -latitude;
                break;
            case 4:  // Longitude
                longitude = parseCoordinate(token, nullptr);
                break;
            case 5:  // E/W
                if (token[0] == 'W') longitude = -longitude;
                break;
            case 6:  // Speed (knots)
                speedKmh = atof(token) * 1.852f;  // Convert knots to km/h
                break;
            case 7:  // Course (degrees)
                heading = atof(token);
                break;
            case 8:  // Date (DDMMYY)
                date = atoi(token);
                break;
        }
        field++;
    }
    
    if (hasFix()) {
        validSentenceCount++;
    }
    
    return true;
}

bool GPS::parseVTG(const char* data) {
    // $GNVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
    char buffer[128];
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token;
    char* ptr = buffer;
    int field = 0;
    
    while ((token = strsep(&ptr, ",")) != nullptr) {
        switch (field) {
            case 0:  // Course (true)
                heading = atof(token);
                break;
            case 6:  // Speed (km/h)
                speedKmh = atof(token);
                break;
        }
        field++;
    }
    
    return true;
}

bool GPS::parseGSA(const char* data) {
    // $GNGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    char buffer[128];
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token;
    char* ptr = buffer;
    int field = 0;
    
    while ((token = strsep(&ptr, ",")) != nullptr) {
        switch (field) {
            case 1:  // Mode (1=no fix, 2=2D, 3=3D)
                // Could track 2D/3D fix here
                break;
            case 14:  // PDOP
                pdop = atof(token);
                break;
            case 15:  // HDOP
                hdop = atof(token);
                break;
            case 16:  // VDOP
                vdop = atof(token);
                break;
        }
        field++;
    }
    
    return true;
}

double GPS::parseCoordinate(const char* str, const char* dir) {
    if (strlen(str) < 3) return 0.0;
    
    double raw = atof(str);
    int deg;
    double min;
    
    // Determine degrees part based on string length (lat vs lon)
    if (strlen(str) > 8) {  // Longitude (DDDMM.MMMM)
        deg = (int)(raw / 100);
        min = raw - deg * 100;
    } else {  // Latitude (DDMM.MMMM)
        deg = (int)(raw / 100);
        min = raw - deg * 100;
    }
    
    return deg + (min / 60.0);
}

bool GPS::validateChecksum(const char* sentence) {
    // Find the checksum separator
    const char* asterisk = strchr(sentence, '*');
    if (!asterisk) return false;
    
    // Calculate checksum
    uint8_t checksum = 0;
    for (const char* p = sentence + 1; p < asterisk; p++) {
        checksum ^= *p;
    }
    
    // Parse expected checksum
    uint8_t expected = (uint8_t)strtol(asterisk + 1, nullptr, 16);
    
    return checksum == expected;
}

bool GPS::waitForFix(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        update();
        if (hasFix()) return true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

bool GPS::setUpdateRate(uint8_t hz) {
    // Calculate interval in milliseconds
    uint16_t interval = 1000 / hz;
    
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "$PMTK220,%d*", interval);
    
    // Calculate checksum
    uint8_t checksum = 0;
    for (char* p = cmd + 1; *p != '*'; p++) {
        checksum ^= *p;
    }
    
    char fullCmd[40];
    snprintf(fullCmd, sizeof(fullCmd), "%s%02X", cmd, checksum);
    
    gpsSerial.println(fullCmd);
    return true;
}

float GPS::getParseSuccessRate() const {
    if (sentenceCount == 0) return 0.0f;
    return (float)validSentenceCount / sentenceCount * 100.0f;
}

bool GPS::isHealthy() const {
    // Check for recent data
    if (millis() - lastFixTime > 5000) return false;
    
    // Check parse success rate
    if (sentenceCount > 100 && getParseSuccessRate() < 50.0f) return false;
    
    // Check fix quality
    if (isFixStale(3000)) return false;
    
    return true;
}

void GPS::fillData(GPSData& data, uint32_t timestamp) {
    data.timestamp_ms = timestamp;
    data.latitude = latitude;
    data.longitude = longitude;
    data.altitude = altitude;
    data.speed_kmh = speedKmh;
    data.heading = heading;
    data.satellites = satellites;
    data.fix_quality = static_cast<uint8_t>(fixType);
    data.hdop = static_cast<uint8_t>(hdop * 10);  // Store as x10 for precision
}

void GPS::printStatus() const {
    DEBUG_PRINTLN(3, "GPS Status:");
    DEBUG_PRINTF(3, "  Fix: %s (%d sats)\n", hasFix() ? "YES" : "NO", satellites);
    DEBUG_PRINTF(3, "  Lat: %.6f, Lon: %.6f\n", latitude, longitude);
    DEBUG_PRINTF(3, "  Speed: %.1f km/h, Heading: %.1f\n", speedKmh, heading);
    DEBUG_PRINTF(3, "  HDOP: %.1f, Accuracy: ~%.1fm\n", hdop, getAccuracy());
    DEBUG_PRINTF(3, "  Sentences: %lu (%.1f%% valid)\n", sentenceCount, getParseSuccessRate());
}

void GPS::resetStats() {
    sentenceCount = 0;
    validSentenceCount = 0;
}
