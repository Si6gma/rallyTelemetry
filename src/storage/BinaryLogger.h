/**
 * High-Performance Binary Data Logger
 * 
 * Features:
 * - Binary format (much smaller than CSV)
 * - Automatic log rotation by size
 * - Double buffering for zero-copy writes
 * - CRC32 checksums for data integrity
 * - Write-ahead indexing for fast seeking
 */

#pragma once

#include "../core/config.h"
#include <SD.h>
#include <SPI.h>

// File header for binary log files
struct __attribute__((packed)) LogFileHeader {
    uint32_t magic;           // 'RLOG'
    uint16_t version;         // File format version
    uint32_t createdTime;     // Unix timestamp
    uint16_t packetSize;      // Expected packet size
    uint16_t reserved;
    char vehicleId[16];       // Vehicle identifier
    char driverName[16];      // Driver name
    uint32_t crc32;           // Header checksum
};

// File statistics
struct LogStats {
    uint32_t packetsWritten;
    uint32_t bytesWritten;
    uint32_t flushCount;
    uint32_t errorCount;
    uint32_t drops;           // Packets dropped due to full buffer
    uint32_t currentFileSize;
    uint8_t currentFileIndex;
};

class BinaryLogger {
private:
    File currentFile;
    bool fileOpen = false;
    
    // Double buffer for batch writes
    static const size_t WRITE_BUFFER_SIZE = 16;  // 16 packets = ~1KB
    TelemetryPacket writeBuffer1[WRITE_BUFFER_SIZE];
    TelemetryPacket writeBuffer2[WRITE_BUFFER_SIZE];
    TelemetryPacket* activeBuffer = writeBuffer1;
    TelemetryPacket* flushBuffer = nullptr;
    size_t bufferCount = 0;
    
    SemaphoreHandle_t bufferMutex = nullptr;
    
    // File rotation
    char currentFilename[32];
    uint8_t fileIndex = 0;
    
    // Statistics
    LogStats stats;
    SemaphoreHandle_t statsMutex = nullptr;
    
    // Configuration
    char vehicleId[17] = "RALLY_CAR_01";
    char driverName[17] = "DRIVER";
    
    // CRC32 table
    static const uint32_t crc32Table[256];
    
    bool openNewFile();
    bool rotateFile();
    void flushBuffer();
    uint32_t calculateCRC32(const void* data, size_t length);
    uint32_t calculatePacketCRC(const TelemetryPacket& packet);
    
public:
    BinaryLogger();
    ~BinaryLogger();
    
    bool begin();
    void end();
    
    // Write packet (non-blocking, buffered)
    bool write(const TelemetryPacket& packet);
    
    // Force flush to SD card
    bool flush();
    
    // File management
    bool setVehicleInfo(const char* vehicle, const char* driver);
    bool rotate();  // Manually rotate to new file
    bool getCurrentFilename(char* buffer, size_t size) const;
    
    // Statistics
    LogStats getStats();
    void resetStats();
    
    // Health check
    bool isHealthy() const;
    float getBufferUtilization() const;
    
    // Maintenance
    bool deleteOldestFile();
    uint8_t countLogFiles() const;
    uint32_t getFreeSpaceMB() const;
    
    // Export functionality
    bool exportToCSV(const char* binFile, const char* csvFile);
};
