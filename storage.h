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
void SDFileWriteln(String filename, String data);
void SDFileCreate(String filename, String data);
bool SDFileExists(String filename);
bool SDRenameFile(String prevFilename, String newFilename);
String formatLogData(double time, double latitude, double longitude, float gForce, int satCount);
String logHeaderData();

#endif