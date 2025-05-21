#include "storage.h"

File file;

void initSD() {
  if (!SD.begin(5)) {
    Serial.println("SD card initialization error.");
    while (1) {}
  }
}

void write(char *filename, char *data) {

}

bool fileExists(char *filename) { 
  return false;
}

void renameFile(char *prevFilename, char* newFilename) {

}