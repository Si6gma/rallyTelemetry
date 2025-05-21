#include "storage.h"

File file;

void initSD() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR IN BEGINNING SD");
    while (1) {}
  }
}

void SDFileWriteln(String filename, String data) {
  file = SD.open(filename, FILE_WRITE);

  if (!file) {
    return;
  }

  file.println(data);

  file.close();
}

bool SDFileExists(String filename) { 
  return SD.exists(filename);
}

bool SDRenameFile(String prevFilename, String newFilename) {
  return SD.rename(prevFilename, newFilename);
}

String formatLogData(double time, double latitude, double longitude, float gForce, int satCount) {
  return String(time, 3) + "," + String(latitude, 6) + "," + String(longitude, 6) + "," + String(gForce) + "," + String(satCount);
}

String logHeaderData() {
  return String("UTC Time,Latitude,Longitude,GForce,satCount");
}
