#include "WiFiTelemetry.h"

WiFiTelemetry::WiFiTelemetry() {
    statsMutex = xSemaphoreCreateMutex();
    
    // Default config
    strncpy(apSSID, WIFI_AP_SSID, 32);
    strncpy(apPassword, WIFI_AP_PASS, 32);
    apSSID[31] = '\0';
    apPassword[31] = '\0';
    
    staSSID[0] = '\0';
    staPassword[0] = '\0';
    
    udpAddress.fromString(TELEMETRY_UDP_HOST);
    udpPort = TELEMETRY_UDP_PORT;
    udpBroadcast = true;
}

WiFiTelemetry::~WiFiTelemetry() {
    end();
    if (statsMutex) vSemaphoreDelete(statsMutex);
}

bool WiFiTelemetry::begin(WiFiMode wifiMode) {
    mode = wifiMode;
    
    // Disconnect any existing connection
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    switch (mode) {
        case WiFiMode::AP_MODE:
            WiFi.mode(WIFI_AP);
            WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_IP, WIFI_AP_NETMASK);
            WiFi.softAP(apSSID, apPassword);
            DEBUG_PRINTF(3, "WiFi AP started: %s\n", apSSID);
            DEBUG_PRINTF(3, "  IP: %s\n", WiFi.softAPIP().toString().c_str());
            break;
            
        case WiFiMode::STA_MODE:
            if (strlen(staSSID) == 0) {
                DEBUG_PRINTLN(1, "STA mode requires SSID configuration!");
                return false;
            }
            WiFi.mode(WIFI_STA);
            WiFi.begin(staSSID, staPassword);
            DEBUG_PRINTF(3, "Connecting to WiFi: %s\n", staSSID);
            
            // Wait for connection (blocking for startup)
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                DEBUG_PRINT(3, ".");
                attempts++;
            }
            DEBUG_PRINTLN(3, "");
            
            if (WiFi.status() != WL_CONNECTED) {
                DEBUG_PRINTLN(1, "WiFi connection failed!");
                return false;
            }
            DEBUG_PRINTF(3, "Connected, IP: %s\n", WiFi.localIP().toString().c_str());
            break;
            
        case WiFiMode::AP_STA_MODE:
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP(apSSID, apPassword);
            if (strlen(staSSID) > 0) {
                WiFi.begin(staSSID, staPassword);
            }
            break;
            
        default:
            return false;
    }
    
    // Start UDP
    udp.begin(udpPort);
    DEBUG_PRINTF(3, "UDP started on port %d\n", udpPort);
    
    // Start mDNS
    if (MDNS.begin("rally-telemetry")) {
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        MDNS.addService("rally", "udp", udpPort);
        DEBUG_PRINTLN(3, "mDNS responder started: rally-telemetry.local");
    }
    
    // Start web server
    webServer = new WebServer(WEB_SERVER_PORT);
    setupWebServer();
    webServer->begin();
    DEBUG_PRINTF(3, "Web server started on port %d\n", WEB_SERVER_PORT);
    
    return true;
}

void WiFiTelemetry::end() {
    udp.stop();
    
    if (webServer) {
        webServer->stop();
        delete webServer;
        webServer = nullptr;
    }
    
    disconnectAllClients();
    
    MDNS.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    mode = WiFiMode::OFF;
}

void WiFiTelemetry::setAPConfig(const char* ssid, const char* password) {
    strncpy(apSSID, ssid, 31);
    strncpy(apPassword, password, 31);
    apSSID[31] = '\0';
    apPassword[31] = '\0';
}

void WiFiTelemetry::setSTAConfig(const char* ssid, const char* password) {
    strncpy(staSSID, ssid, 31);
    strncpy(staPassword, password, 31);
    staSSID[31] = '\0';
    staPassword[31] = '\0';
}

void WiFiTelemetry::setUDPEndpoint(const char* ip, uint16_t port, bool broadcast) {
    udpAddress.fromString(ip);
    udpPort = port;
    udpBroadcast = broadcast;
}

bool WiFiTelemetry::stream(const TelemetryPacket& packet) {
    return streamRaw((const uint8_t*)&packet, sizeof(packet));
}

bool WiFiTelemetry::streamRaw(const uint8_t* data, size_t len) {
    if (mode == WiFiMode::OFF) return false;
    
    // Rate limiting
    uint32_t now = millis();
    if (now - lastPacketTime < minIntervalMs) {
        return true;  // Skip but don't error
    }
    lastPacketTime = now;
    
    // UDP broadcast/multicast
    if (udpBroadcast) {
        udp.beginPacketMulticast(udpAddress, udpPort, WiFi.localIP());
    } else {
        udp.beginPacket(udpAddress, udpPort);
    }
    udp.write(data, len);
    bool success = udp.endPacket() == 1;
    
    if (success) {
        xSemaphoreTake(statsMutex, portMAX_DELAY);
        stats.packetsSent++;
        stats.bytesSent += len;
        xSemaphoreGive(statsMutex);
    } else {
        stats.errors++;
    }
    
    // Also send to TCP clients
    updateTCPClients();
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i].connected()) {
            tcpClients[i].write(data, len);
        }
    }
    
    return success;
}

void WiFiTelemetry::updateTCPClients() {
    // Clean up disconnected clients
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i] && !tcpClients[i].connected()) {
            tcpClients[i].stop();
            DEBUG_PRINTF(3, "TCP client %d disconnected\n", i);
        }
    }
}

void WiFiTelemetry::disconnectAllClients() {
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i]) {
            tcpClients[i].stop();
        }
    }
}

int WiFiTelemetry::getConnectedClientCount() const {
    int count = 0;
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i].connected()) count++;
    }
    return count;
}

bool WiFiTelemetry::isConnected() const {
    if (mode == WiFiMode::OFF) return false;
    if (mode == WiFiMode::AP_MODE) return true;  // AP is always "connected"
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiTelemetry::getLocalIP() const {
    if (mode == WiFiMode::AP_MODE || mode == WiFiMode::AP_STA_MODE) {
        return WiFi.softAPIP();
    }
    return WiFi.localIP();
}

String WiFiTelemetry::getModeString() const {
    switch (mode) {
        case WiFiMode::OFF: return "OFF";
        case WiFiMode::AP_MODE: return "AP";
        case WiFiMode::STA_MODE: return "STA";
        case WiFiMode::AP_STA_MODE: return "AP+STA";
        default: return "UNKNOWN";
    }
}

TelemetryStats WiFiTelemetry::getStats() {
    TelemetryStats s;
    xSemaphoreTake(statsMutex, portMAX_DELAY);
    s = stats;
    xSemaphoreGive(statsMutex);
    return s;
}

void WiFiTelemetry::resetStats() {
    xSemaphoreTake(statsMutex, portMAX_DELAY);
    memset(&stats, 0, sizeof(stats));
    xSemaphoreGive(statsMutex);
}

void WiFiTelemetry::handleWebClient() {
    if (webServer) {
        webServer->handleClient();
    }
    MDNS.update();
}

void WiFiTelemetry::setupWebServer() {
    webServer->on("/", HTTP_GET, [this]() { handleRoot(); });
    webServer->on("/status", HTTP_GET, [this]() { handleStatus(); });
    webServer->on("/config", HTTP_POST, [this]() { handleConfig(); });
    webServer->on("/download", HTTP_GET, [this]() { handleDownload(); });
}

void WiFiTelemetry::handleRoot() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Rally Telemetry</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #1a1a1a; color: #fff; }
        h1 { color: #ff6b35; }
        .card { background: #2a2a2a; padding: 15px; margin: 10px 0; border-radius: 8px; }
        .metric { display: inline-block; margin: 10px 20px 10px 0; }
        .label { color: #888; font-size: 12px; }
        .value { font-size: 24px; font-weight: bold; color: #4CAF50; }
        .alert { color: #f44336; }
    </style>
</head>
<body>
    <h1>Rally Telemetry Pro</h1>
    <div class="card">
        <h2>System Status</h2>
        <div class="metric">
            <div class="label">WiFi Mode</div>
            <div class="value">)" + getModeString() + R"(</div>
        </div>
        <div class="metric">
            <div class="label">IP Address</div>
            <div class="value">)" + getLocalIP().toString() + R"(</div>
        </div>
        <div class="metric">
            <div class="label">Signal</div>
            <div class="value">)" + signalStrengthToString(getSignalStrength()) + R"(</div>
        </div>
    </div>
    <div class="card">
        <h2>Actions</h2>
        <a href="/download" style="color:#4CAF50">Download Logs</a> | 
        <a href="/status" style="color:#4CAF50">View Status (JSON)</a>
    </div>
</body>
</html>
)";
    webServer->send(200, "text/html", html);
}

void WiFiTelemetry::handleStatus() {
    String json = "{";
    json += "\"mode\":\"" + getModeString() + "\",";
    json += "\"ip\":\"" + getLocalIP().toString() + "\",";
    json += "\"rssi\":" + String(getSignalStrength()) + ",";
    json += "\"connected\":" + String(isConnected() ? "true" : "false");
    json += "}";
    webServer->send(200, "application/json", json);
}

void WiFiTelemetry::handleConfig() {
    // Handle configuration updates
    webServer->send(200, "text/plain", "Configuration updated");
}

void WiFiTelemetry::handleDownload() {
    // List available log files
    String html = "<h1>Log Files</h1><ul>";
    for (uint8_t i = 0; i < MAX_LOG_FILES; i++) {
        char filename[32];
        snprintf(filename, sizeof(filename), "%s_%03d%s", LOG_FILE_BASE, i, LOG_EXT);
        if (SD.exists(filename)) {
            html += "<li><a href=\"/download/" + String(filename) + "\">" + String(filename) + "</a></li>";
        }
    }
    html += "</ul>";
    webServer->send(200, "text/html", html);
}

String WiFiTelemetry::signalStrengthToString(int rssi) {
    if (rssi > -50) return "Excellent";
    if (rssi > -60) return "Good";
    if (rssi > -70) return "Fair";
    if (rssi > -80) return "Weak";
    return "Poor";
}

int WiFiTelemetry::getSignalStrength() {
    if (mode == WiFiMode::STA_MODE || mode == WiFiMode::AP_STA_MODE) {
        return WiFi.RSSI();
    }
    return 0;
}
