#include "WiFiTelemetry.h"
#include <SD.h>

WiFiTelemetry::WiFiTelemetry() {
    statsMutex = xSemaphoreCreateMutex();
    packetMutex = xSemaphoreCreateMutex();
    
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
    
    memset(&lastPacket, 0, sizeof(lastPacket));
}

WiFiTelemetry::~WiFiTelemetry() {
    end();
    if (statsMutex) vSemaphoreDelete(statsMutex);
    if (packetMutex) vSemaphoreDelete(packetMutex);
}

bool WiFiTelemetry::begin(WiFiMode wifiMode) {
    mode = wifiMode;
    
    // Initialize SPIFFS for serving dashboard files
    if (!SPIFFS.begin(true)) {
        DEBUG_PRINTLN(2, "SPIFFS initialization failed - dashboard may not work");
        // Continue anyway - UDP still works
    } else {
        DEBUG_PRINTLN(3, "SPIFFS initialized");
        // Check for dashboard files
        if (SPIFFS.exists("/dashboard/index.html")) {
            DEBUG_PRINTLN(3, "Dashboard files found in SPIFFS");
        }
    }
    
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
            
        case WiFiMode::STA_MODE: {
            if (strlen(staSSID) == 0) {
                DEBUG_PRINTLN(1, "STA mode requires SSID configuration!");
                return false;
            }
            WiFi.mode(WIFI_STA);
            WiFi.begin(staSSID, staPassword);
            DEBUG_PRINTF(3, "Connecting to WiFi: %s\n", staSSID);
            
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
        }
            
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
    DEBUG_PRINTLN(3, "Dashboard: http://rally-telemetry.local or http://192.168.4.1");
    
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
    
    SPIFFS.end();
    
    mode = WiFiMode::OFF;
}

void WiFiTelemetry::setupWebServer() {
    // Dashboard routes
    webServer->on("/", HTTP_GET, [this]() { handleRoot(); });
    webServer->on("/dashboard", HTTP_GET, [this]() { handleDashboard(); });
    webServer->on("/dashboard/", HTTP_GET, [this]() { handleDashboard(); });
    
    // API routes
    webServer->on("/status", HTTP_GET, [this]() { handleStatus(); });
    webServer->on("/api/live", HTTP_GET, [this]() { handleLiveData(); });
    webServer->on("/api/files", HTTP_GET, [this]() { handleListFiles(); });
    webServer->on("/api/convert", HTTP_GET, [this]() { handleConvertBinary(); });
    webServer->on("/config", HTTP_POST, [this]() { handleConfig(); });
    webServer->on("/download", HTTP_GET, [this]() { handleDownload(); });
    
    // Static files (CSS, JS)
    webServer->onNotFound([this]() { handleStaticFile(); });
}

void WiFiTelemetry::handleRoot() {
    // Redirect to dashboard
    webServer->sendHeader("Location", "/dashboard");
    webServer->send(302, "text/plain", "Redirecting to dashboard...");
}

void WiFiTelemetry::handleDashboard() {
    // Serve the dashboard index.html
    if (!serveFile("/dashboard/index.html")) {
        // Fallback if SPIFFS not available
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Rally Telemetry Pro</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; margin: 0; padding: 20px; background: #1a1a1a; color: #fff; }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { color: #ff6b35; }
        .card { background: #2a2a2a; padding: 20px; margin: 15px 0; border-radius: 12px; }
        .metric { display: inline-block; margin: 15px 25px 15px 0; }
        .label { color: #888; font-size: 13px; text-transform: uppercase; }
        .value { font-size: 32px; font-weight: 700; color: #4CAF50; }
        .value.warning { color: #ff9800; }
        .value.critical { color: #f44336; }
        a { color: #4CAF50; }
        .btn { background: #4CAF50; color: white; padding: 12px 24px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; display: inline-block; margin: 5px; }
        .btn:hover { background: #45a049; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Rally Telemetry Pro</h1>
        <p>Real-time rally car telemetry system</p>
        
        <div class="card">
            <h2>System Status</h2>
            <div class="metric">
                <div class="label">WiFi Mode</div>
                <div class="value" id="wifiMode">)" + getModeString() + R"(</div>
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
            <h2>Live Data</h2>
            <div class="metric">
                <div class="label">G-Force</div>
                <div class="value" id="gforce">--</div>
            </div>
            <div class="metric">
                <div class="label">Speed</div>
                <div class="value" id="speed">-- km/h</div>
            </div>
            <div class="metric">
                <div class="label">Sats</div>
                <div class="value" id="sats">--</div>
            </div>
        </div>
        
        <div class="card">
            <h2>Actions</h2>
            <a href="/api/files" class="btn">Download Logs</a>
            <a href="/api/live" class="btn">Live API</a>
            <a href="/status" class="btn">System Status (JSON)</a>
        </div>
        
        <p><small>v)" + String(FIRMWARE_VERSION) + R"( | <a href="https://github.com/Si6gma/rallyTelemetry">GitHub</a></small></p>
    </div>
    <script>
        async function updateLiveData() {
            try {
                const resp = await fetch('/api/live');
                const data = await resp.json();
                const g = Math.sqrt(data.imu.ax**2 + data.imu.ay**2 + data.imu.az**2) / 9.81;
                document.getElementById('gforce').textContent = g.toFixed(2) + 'G';
                document.getElementById('speed').textContent = data.gps.speed.toFixed(1) + ' km/h';
                document.getElementById('sats').textContent = data.gps.sats;
            } catch(e) {}
        }
        setInterval(updateLiveData, 500);
        updateLiveData();
    </script>
</body>
</html>
)";
        webServer->send(200, "text/html", html);
    }
}

void WiFiTelemetry::handleStaticFile() {
    String path = webServer->uri();
    if (path.endsWith("/")) path += "index.html";
    
    // Map /dashboard/* to SPIFFS /dashboard/*
    if (path.startsWith("/dashboard/")) {
        String spiffsPath = path;
        if (serveFile(spiffsPath)) return;
    }
    
    handleNotFound();
}

bool WiFiTelemetry::serveFile(const String& path) {
    if (!SPIFFS.exists(path)) {
        return false;
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        return false;
    }
    
    String contentType = getContentType(path);
    
    // Handle gzip compression
    if (SPIFFS.exists(path + ".gz")) {
        file.close();
        file = SPIFFS.open(path + ".gz", "r");
        webServer->sendHeader("Content-Encoding", "gzip");
    }
    
    webServer->streamFile(file, contentType);
    file.close();
    return true;
}

String WiFiTelemetry::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";
    if (filename.endsWith(".png")) return "image/png";
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    if (filename.endsWith(".gif")) return "image/gif";
    if (filename.endsWith(".svg")) return "image/svg+xml";
    if (filename.endsWith(".ico")) return "image/x-icon";
    if (filename.endsWith(".gz")) return "application/gzip";
    return "text/plain";
}

void WiFiTelemetry::handleStatus() {
    String json = "{";
    json += "\"mode\":\"" + getModeString() + "\",";
    json += "\"ip\":\"" + getLocalIP().toString() + "\",";
    json += "\"rssi\":" + String(getSignalStrength()) + ",";
    json += "\"connected\":" + String(isConnected() ? "true" : "false") + ",";
    json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += "}";
    webServer->send(200, "application/json", json);
}

void WiFiTelemetry::handleLiveData() {
    xSemaphoreTake(packetMutex, portMAX_DELAY);
    
    // Convert last packet to JSON
    String json = "{";
    json += "\"timestamp\":" + String(lastPacket.timestamp_ms) + ",";
    json += "\"sequence\":" + String(lastPacket.sequence) + ",";
    json += "\"imu\":{";
    json += "\"ax\":" + String(lastPacket.imu.accel_x, 3) + ",";
    json += "\"ay\":" + String(lastPacket.imu.accel_y, 3) + ",";
    json += "\"az\":" + String(lastPacket.imu.accel_z, 3) + ",";
    json += "\"gx\":" + String(lastPacket.imu.gyro_x, 3) + ",";
    json += "\"gy\":" + String(lastPacket.imu.gyro_y, 3) + ",";
    json += "\"gz\":" + String(lastPacket.imu.gyro_z, 3) + ",";
    json += "\"temp\":" + String(lastPacket.imu.temperature, 1);
    json += "},\"gps\":{";
    json += "\"lat\":" + String(lastPacket.gps.latitude, 6) + ",";
    json += "\"lon\":" + String(lastPacket.gps.longitude, 6) + ",";
    json += "\"alt\":" + String(lastPacket.gps.altitude, 1) + ",";
    json += "\"speed\":" + String(lastPacket.gps.speed_kmh, 1) + ",";
    json += "\"heading\":" + String(lastPacket.gps.heading, 1) + ",";
    json += "\"sats\":" + String(lastPacket.gps.satellites) + ",";
    json += "\"fix\":" + String(lastPacket.gps.fix_quality);
    json += "}}";
    
    xSemaphoreGive(packetMutex);
    
    webServer->send(200, "application/json", json);
}

void WiFiTelemetry::handleListFiles() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Rally Telemetry - Files</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; margin: 0; padding: 20px; background: #1a1a1a; color: #fff; }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { color: #ff6b35; }
        .file { background: #2a2a2a; padding: 15px; margin: 10px 0; border-radius: 8px; display: flex; justify-content: space-between; align-items: center; }
        .file-info { flex: 1; }
        .file-name { font-weight: 600; color: #4CAF50; }
        .file-size { color: #888; font-size: 13px; }
        .btn { background: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; margin-left: 10px; }
        .btn.secondary { background: #666; }
        .btn:hover { opacity: 0.9; }
        a { color: #4CAF50; }
    </style>
</head>
<body>
    <div class="container">
        <h1>📁 Log Files</h1>
        <p><a href="/">← Back to Dashboard</a></p>
)";

    // List binary files
    for (uint8_t i = 0; i < MAX_LOG_FILES; i++) {
        char filename[32];
        snprintf(filename, sizeof(filename), "%s_%03d%s", LOG_FILE_BASE, i, LOG_EXT);
        if (SD.exists(filename)) {
            File f = SD.open(filename);
            size_t size = f.size();
            f.close();
            
            String sizeStr;
            if (size < 1024) sizeStr = String(size) + " B";
            else if (size < 1024*1024) sizeStr = String(size/1024) + " KB";
            else sizeStr = String(size/(1024*1024)) + " MB";
            
            html += "<div class='file'>";
            html += "<div class='file-info'>";
            html += "<div class='file-name'>" + String(filename) + "</div>";
            html += "<div class='file-size'>" + sizeStr + "</div>";
            html += "</div>";
            html += "<div>";
            html += "<a href='/download?file=" + String(filename) + "' class='btn'>Download BIN</a>";
            html += "<a href='/api/convert?file=" + String(filename) + "' class='btn secondary'>Download CSV</a>";
            html += "</div>";
            html += "</div>";
        }
    }

    html += R"(
        <p style="margin-top: 30px; color: #888;">
            <small>Binary files are compact and fast. CSV files are human-readable and compatible with Excel/sheets.</small>
        </p>
    </div>
</body>
</html>
)";
    
    webServer->send(200, "text/html", html);
}

void WiFiTelemetry::handleConvertBinary() {
    if (!webServer->hasArg("file")) {
        webServer->send(400, "text/plain", "Missing file parameter");
        return;
    }
    
    String filename = webServer->arg("file");
    if (!filename.startsWith("/")) filename = "/" + filename;
    
    if (!SD.exists(filename)) {
        webServer->send(404, "text/plain", "File not found");
        return;
    }
    
    // Convert to CSV
    String csvData;
    if (!convertBinaryToCSV(filename.c_str(), csvData)) {
        webServer->send(500, "text/plain", "Conversion failed");
        return;
    }
    
    String csvFilename = filename.substring(0, filename.lastIndexOf('.')) + ".csv";
    
    webServer->sendHeader("Content-Disposition", "attachment; filename=\"" + csvFilename.substring(1) + "\"");
    webServer->send(200, "text/csv", csvData);
}

bool WiFiTelemetry::convertBinaryToCSV(const char* binPath, String& csvOutput) {
    File binFile = SD.open(binPath, FILE_READ);
    if (!binFile) return false;
    
    // Read header
    struct __attribute__((packed)) LogFileHeader {
        uint32_t magic;
        uint16_t version;
        uint32_t createdTime;
        uint16_t packetSize;
        uint16_t reserved;
        char vehicleId[16];
        char driverName[16];
        uint32_t crc32;
    } header;
    
    if (binFile.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
        binFile.close();
        return false;
    }
    
    // CSV header
    csvOutput = "Timestamp,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,TempC,";
    csvOutput += "Latitude,Longitude,Altitude,SpeedKmh,Heading,Satellites,FixQuality\n";
    
    // Read packets
    TelemetryPacket packet;
    while (binFile.read((uint8_t*)&packet, sizeof(packet)) == sizeof(packet)) {
        if (packet.magic != PACKET_MAGIC) continue;
        
        csvOutput += String(packet.timestamp_ms) + ",";
        csvOutput += String(packet.imu.accel_x, 3) + ",";
        csvOutput += String(packet.imu.accel_y, 3) + ",";
        csvOutput += String(packet.imu.accel_z, 3) + ",";
        csvOutput += String(packet.imu.gyro_x, 3) + ",";
        csvOutput += String(packet.imu.gyro_y, 3) + ",";
        csvOutput += String(packet.imu.gyro_z, 3) + ",";
        csvOutput += String(packet.imu.temperature, 1) + ",";
        csvOutput += String(packet.gps.latitude, 6) + ",";
        csvOutput += String(packet.gps.longitude, 6) + ",";
        csvOutput += String(packet.gps.altitude, 1) + ",";
        csvOutput += String(packet.gps.speed_kmh, 1) + ",";
        csvOutput += String(packet.gps.heading, 1) + ",";
        csvOutput += String(packet.gps.satellites) + ",";
        csvOutput += String(packet.gps.fix_quality) + "\n";
    }
    
    binFile.close();
    return true;
}

void WiFiTelemetry::handleDownload() {
    if (!webServer->hasArg("file")) {
        handleListFiles();
        return;
    }
    
    String filename = webServer->arg("file");
    if (!filename.startsWith("/")) filename = "/" + filename;
    
    if (!SD.exists(filename)) {
        webServer->send(404, "text/plain", "File not found");
        return;
    }
    
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        webServer->send(500, "text/plain", "Cannot open file");
        return;
    }
    
    webServer->sendHeader("Content-Disposition", "attachment; filename=\"" + filename.substring(1) + "\"");
    webServer->streamFile(file, "application/octet-stream");
    file.close();
}

void WiFiTelemetry::handleConfig() {
    // Handle configuration updates via POST
    if (webServer->hasArg("ssid")) {
        setAPConfig(webServer->arg("ssid").c_str(), 
                    webServer->arg("password").c_str());
    }
    
    webServer->send(200, "application/json", "{\"success\":true}");
}

void WiFiTelemetry::handleNotFound() {
    webServer->send(404, "text/plain", "Not Found");
}

void WiFiTelemetry::updateLiveData(const TelemetryPacket& packet) {
    xSemaphoreTake(packetMutex, portMAX_DELAY);
    memcpy(&lastPacket, &packet, sizeof(packet));
    xSemaphoreGive(packetMutex);
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
    // Update web API data
    updateLiveData(packet);
    
    return streamRaw((const uint8_t*)&packet, sizeof(packet));
}

bool WiFiTelemetry::streamRaw(const uint8_t* data, size_t len) {
    if (mode == WiFiMode::OFF) return false;
    
    // Rate limiting
    uint32_t now = millis();
    if (now - lastPacketTime < minIntervalMs) {
        return true;
    }
    lastPacketTime = now;
    
    // UDP broadcast/multicast
    udp.beginPacket(udpAddress, udpPort);
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
    
    // TCP clients
    updateTCPClients();
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i].connected()) {
            tcpClients[i].write(data, len);
        }
    }
    
    return success;
}

void WiFiTelemetry::updateTCPClients() {
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i] && !tcpClients[i].connected()) {
            tcpClients[i].stop();
            DEBUG_PRINTF(4, "TCP client %d disconnected\n", i);
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

int WiFiTelemetry::getConnectedClientCount() {
    int count = 0;
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (tcpClients[i].connected()) count++;
    }
    return count;
}

bool WiFiTelemetry::isConnected() const {
    if (mode == WiFiMode::OFF) return false;
    if (mode == WiFiMode::AP_MODE) return true;
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
