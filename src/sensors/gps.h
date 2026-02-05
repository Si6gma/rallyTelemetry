#ifndef GPS_H
#define GPS_H

#include "../core/Sensor.h"
#include "../core/config.h"
#include <HardwareSerial.h>

class GPS : public Sensor {
private:
    HardwareSerial gpsSerial;
    double latitude;
    double longitude;
    int satCount;
    int fixQuality;
    String time;
    
    char gpsBuffer[GPS_BUFFER_SIZE];
    int bufferIndex;

    void parseGNGGASentence(const char *sentence);
    double convertToDecimalDegrees(double raw);

public:
    GPS();
    bool begin() override;
    void update() override;
    void printHeaderCSV(Print& p) override;
    void printDataCSV(Print& p) override;
};

#endif // GPS_H
