#include "storage.h"

File file;

void initSD() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS))
    Serial.println("ERROR IN BEGINNING SD");
  while (1) {
  }
  {
  }

  void write(char *filename, char *data) {
  }

  bool fileExists(char *filename) {
    return false;
  }

  void renameFile(char *prevFilename, char *newFilename) {
  }

  String stringData() {
    return String(gpsData.time, 2) + "," + String(gpsData.latitude, 6) + "," + String(gpsData.longitude, 6) + "," + String(getGForce()) + "," + String(gpsData.satCount);
  }

  String headerData() {
    return String("UTC Time,Latitude,Longitude,G-Force,satCount");
  }