/**
 * Advanced GPS Module (NEO-M8N or compatible)
 * 
 * Features:
 * - Multi-sentence NMEA parsing (GGA, RMC, VTG)
 * - 10Hz update rate configuration
n * - HDOP and satellite quality metrics
 * - Automatic baud rate detection
 * - PPS (Pulse Per Second) support for timing
 */

#pragma once

#include "../core/config.h"
#include <HardwareSerial.h>

// GPS fix quality
enum class GPSFixType : uint8_t {
    NO_FIX = 0,
    GPS_FIX = 1,
    DGPS_FIX = 2,
    PPS_FIX = 3,
    RTK_FIXED = 4,
    RTK_FLOAT = 5,
    ESTIMATED = 6,
    MANUAL = 7,
    SIMULATION = 8
};

// GPS navigation status
enum class GPSNavStatus : uint8_t {
    UNKNOWN = 0,
    VALID = 1,
    WARNING = 2,
    ERROR = 3
};

class GPS {
private:
    HardwareSerial gpsSerial;
    
    // Raw NMEA buffer
    char nmeaBuffer[256];
    uint8_t bufferIndex = 0;
    
    // Parsed data
    double latitude = 0.0;      // Decimal degrees
    double longitude = 0.0;     // Decimal degrees
    float altitude = 0.0f;      // Meters above sea level
    float speedKmh = 0.0f;      // km/h
    float heading = 0.0f;       // Degrees true
    float hdop = 99.9f;         // Horizontal dilution of precision
    float vdop = 99.9f;         // Vertical DOP
    float pdop = 99.9f;         // Position DOP
    
    uint8_t satellites = 0;
    GPSFixType fixType = GPSFixType::NO_FIX;
    GPSNavStatus navStatus = GPSNavStatus::UNKNOWN;
    
    // Time
    uint8_t hours = 0;
    uint8_t minutes = 0;
    float seconds = 0.0f;
    uint32_t date = 0;          // DDMMYY format
    
    // Quality metrics
    uint32_t lastFixTime = 0;
    uint32_t fixAge = 0;
    uint32_t sentenceCount = 0;
    uint32_t validSentenceCount = 0;
    
    // Configuration
    bool configured = false;
    
    // NMEA parsing
    bool parseNMEA(const char* sentence);
    bool parseGGA(const char* data);
    bool parseRMC(const char* data);
    bool parseVTG(const char* data);
    bool parseGSA(const char* data);
    bool parseGSV(const char* data);
    
    double parseCoordinate(const char* str, const char* dir);
    bool validateChecksum(const char* sentence);
    
public:
    GPS();
    
    bool begin(int baudRate = GPS_BAUD_RATE);
    void end();
    
    // Process incoming data - call frequently or from task
    void update();
    
    // Wait for valid fix with timeout
    bool waitForFix(uint32_t timeoutMs);
    
    // Configuration
    bool setUpdateRate(uint8_t hz);
    bool setNMEASentences(bool gga, bool rmc, bool vtg, bool gsa);
    
    // Data accessors
    double getLatitude() const { return latitude; }
    double getLongitude() const { return longitude; }
    float getAltitude() const { return altitude; }
    float getSpeedKmh() const { return speedKmh; }
    float getSpeedMs() const { return speedKmh / 3.6f; }
    float getHeading() const { return heading; }
    float getHDOP() const { return hdop; }
    
    uint8_t getSatellites() const { return satellites; }
    GPSFixType getFixType() const { return fixType; }
    bool hasFix() const { return fixType != GPSFixType::NO_FIX; }
    
    uint32_t getFixAge() const { return fixAge; }
    bool isFixStale(uint32_t maxAgeMs = 2000) const { return fixAge > maxAgeMs; }
    
    // Time accessors
    void getTime(uint8_t& h, uint8_t& m, float& s) const { h = hours; m = minutes; s = seconds; }
    uint32_t getDate() const { return date; }
    
    // Quality metrics
    float getAccuracy() const { return hdop * 5.0f; }  // Rough accuracy estimate in meters
    uint32_t getSentenceCount() const { return sentenceCount; }
    float getParseSuccessRate() const;
    
    // Statistics
    void resetStats();
    
    // Health check
    bool isHealthy() const;
    
    // Convert to packet format
    void fillData(GPSData& data, uint32_t timestamp);
    
    // Debug
    void printStatus() const;
};
