#include "storage.h"

File file;

void initSD() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR IN BEGINNING SD");
    while (1) {}
  }
}

void SDWriteln(char *filename, String data) {
  file = SD.open(filename, FILE_APPEND);

  file.println(data);
}

bool SDFileExists(char *filename) { 
  return SD.exists(filename);
}

bool SDRenameFile(char *prevFilename, char* newFilename) {
  return SD.rename(prevFilename, newFilename);
}

String formatLogData() {
  return String(gpsData.time, 2) + "," + String(gpsData.latitude, 6) + "," + String(gpsData.longitude, 6) + "," + String(getGForce()) + "," + String(gpsData.satCount);
}

String logReaderData() {
  return String("UTC Time,Latitude,Longitude,G-Force,satCount");
}
