/**
 * WiFi Telemetry Streaming Module with Web Dashboard
 * 
 * Features:
 * - UDP broadcast/multicast streaming
 * - Web dashboard for real-time visualization
 * - Serve static files from SPIFFS
 * - Binary to CSV conversion API
 * - TCP server for reliable data transfer
 * - mDNS service discovery
 */

#pragma once

#include "../core/config.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

// Connection modes
enum class WiFiMode : uint8_t {
    OFF = 0,
    AP_MODE,       // Create AP for direct connection
    STA_MODE,      // Connect to existing network
    AP_STA_MODE    // Both
};

// Client connection state
struct ClientInfo {
    IPAddress ip;
    uint32_t connectedTime;
    uint32_t packetsSent;
    uint32_t bytesSent;
    bool isActive;
};

// Telemetry statistics
struct TelemetryStats {
    uint32_t packetsSent;
    uint32_t bytesSent;
    uint32_t clientsConnected;
    uint32_t errors;
    float avgLatency;
};

class WiFiTelemetry {
private:
    WiFiMode mode = WiFiMode::OFF;
    WiFiUDP udp;
    WebServer* webServer = nullptr;
    
    // Configuration
    char apSSID[32];
    char apPassword[32];
    char staSSID[32];
    char staPassword[32];
    
    // UDP settings
    IPAddress udpAddress;
    uint16_t udpPort;
    bool udpBroadcast;
    
    // TCP clients
    static const int MAX_TCP_CLIENTS = 4;
    WiFiClient tcpClients[MAX_TCP_CLIENTS];
    
    // Statistics
    TelemetryStats stats;
    SemaphoreHandle_t statsMutex = nullptr;
    
    // Rate limiting
    uint32_t lastPacketTime = 0;
    uint32_t minIntervalMs = 50;  // Max 20Hz
    
    // Current telemetry data for web API
    TelemetryPacket lastPacket;
    SemaphoreHandle_t packetMutex = nullptr;
    
    // Web server handlers
    void setupWebServer();
    void handleRoot();
    void handleDashboard();
    void handleStaticFile();
    void handleStatus();
    void handleLiveData();
    void handleConfig();
    void handleListFiles();
    void handleDownload();
    void handleConvertBinary();
    void handleNotFound();
    
    // File serving
    String getContentType(const String& filename);
    bool serveFile(const String& path);
    
    // Binary to CSV conversion
    bool convertBinaryToCSV(const char* binPath, String& csvOutput);
    
public:
    WiFiTelemetry();
    ~WiFiTelemetry();
    
    bool begin(WiFiMode wifiMode = WiFiMode::AP_MODE);
    void end();
    
    // Configuration
    void setAPConfig(const char* ssid, const char* password);
    void setSTAConfig(const char* ssid, const char* password);
    void setUDPEndpoint(const char* ip, uint16_t port, bool broadcast = true);
    
    // Main streaming function
    bool stream(const TelemetryPacket& packet);
    
    // Alternative: Raw binary stream
    bool streamRaw(const uint8_t* data, size_t len);
    
    // Update current packet for web API
    void updateLiveData(const TelemetryPacket& packet);
    
    // TCP server management
    void updateTCPClients();
    void disconnectAllClients();
    int getConnectedClientCount() const;
    
    // Status
    bool isConnected() const;
    IPAddress getLocalIP() const;
    String getModeString() const;
    TelemetryStats getStats();
    void resetStats();
    
    // Web interface
    void handleWebClient();
    
    // Utility
    static String signalStrengthToString(int rssi);
    static int getSignalStrength();
};
