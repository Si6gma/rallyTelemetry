#ifndef STORAGE_H
#define STORAGE_H

#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include "gps.h"

#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS 5

void initSD();
void SDWriteln(char *filename, String data);
bool SDFileExists(char *filename);
bool SDRenameFile(char *prevFilename, char *newFilename);

#endif