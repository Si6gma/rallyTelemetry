#ifndef STORAGE_H
#define STORAGE_H

#include <SD.h>
#include <SPI.h>
#ifdef ESP32
#include <FS.h>
#endif
#include "../core/config.h"

class Storage {
public:
    Storage();
    bool begin();
    
    // Log data using the Print interface to avoid Strings
    // We expose the underlying File object so SensorManager can write directly to it
    // This is much faster than passing Strings around.
    Print* getPrintInterface(); 
    
    void flush(); // Force partial save (sync)
    void close(); // Close properly
    
    bool fileExists(const char* filename);
    void setFilename(const char* filename);

private:
   File file;
   String currentFilename;
   unsigned long lastFlushTime;
   const unsigned long FLUSH_INTERVAL = 1000; // Flush every 1 second
};

#endif // STORAGE_H