#include "storage.h"

Storage::Storage() {
    currentFilename = "/log.csv";
}

bool Storage::begin() {
    SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN)) {
        return false;
    }
    // Open the file immediately if filename is set
    if (currentFilename.length() > 0) {
        file = SD.open(currentFilename.c_str(), FILE_APPEND);
    }
    return file; 
}

void Storage::setFilename(const char* filename) {
    if (file) file.close();
    currentFilename = String(filename);
    if (SD.begin(SD_CS_PIN)) { // Ensure SD is ready
        file = SD.open(currentFilename.c_str(), FILE_APPEND);
    }
}

Print* Storage::getPrintInterface() {
    // Re-open if closed (error handling)
    if (!file && currentFilename.length() > 0) {
         file = SD.open(currentFilename.c_str(), FILE_APPEND);
    }
    return &file;
}

void Storage::flush() {
    if (file) {
        // SD library flush() syncs data to card
        file.flush();
    }
}

void Storage::close() {
    if (file) file.close();
}

// Removed generic log(String) to discourage its use, or we can keep it for legacy
bool Storage::fileExists(const char* filename) {
    return SD.exists(filename);
}
