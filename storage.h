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
void write(char *filename, char *data);
bool fileExists(char *filename);
void renameFile(char *prevFilename, char *newFilename);

#endif