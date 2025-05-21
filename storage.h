#ifndef STORAGE_H
#define STORAGE_H

#include <SD.h>
#include <SPI.h>
#include <FS.h>

void initSD();
void write(char *filename, char *data);
bool fileExists(char *filename);
void renameFile(char *prevFilename, char *newFilename);

#endif