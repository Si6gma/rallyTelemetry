#include "storage.h"

File file;

void initSD() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS))
    Serial.println("ERROR IN BEGINNING SD");
    while (1) {}
  {
}

void write(char *filename, char *data) {

}

bool fileExists(char *filename) { 
  return false;
}

void renameFile(char *prevFilename, char* newFilename) {

}