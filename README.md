# Rally Telemetry

High-performance telemetry data logger for rally cars. Real-time sensor fusion, dual-core processing, web dashboard, and alerts.

[![PlatformIO](https://img.shields.io/badge/platformio-esp32-blue)](https://platformio.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Features

- **100Hz IMU sampling** - 6-axis accelerometer/gyroscope
- **10Hz GPS tracking** - Position, speed, altitude
- **50Hz binary logging** - Compressed format with CRC32
- **Real-time alerts** - G-force, roll, pitch thresholds
- **Web dashboard** - Live visualization at 192.168.4.1
- **WiFi streaming** - UDP telemetry broadcast
- **Automatic log rotation** - 50MB chunks, circular buffer
- **Dual-core RTOS** - Sensors on Core 0, I/O on Core 1

## Hardware

| Component | Part | Interface |
|-----------|------|-----------|
| Microcontroller | ESP32 DevKit | - |
| IMU | MPU6050 | I2C (GPIO 21/22) |
| GPS | NEO-M8N | UART2 (GPIO 16/17) |
| Storage | MicroSD | SPI (GPIO 5/18/19/23) |
| Status LED | RGB Common Cathode | GPIO 25/26/27 |

## Quick Start

### PlatformIO

```bash
# Clone and build
git clone https://github.com/Si6gma/rallyTelemetry.git
cd rallyTelemetry

# Upload firmware
pio run --target upload

# Upload dashboard files
pio run --target uploadfs

# Monitor
pio device monitor
```

### Arduino IDE

1. Install ESP32 board support
2. Install libraries: `Adafruit MPU6050`, `Adafruit Unified Sensor`
3. Use ESP32 Sketch Data Upload plugin for dashboard files
4. Select ESP32 Dev Module, 240MHz CPU
5. Upload

## Usage

1. **Power on** - Blue LED blinks during initialization
2. **Keep still** - IMU auto-calibrates
3. **Wait for GPS** - Green LED when ready
4. **Recording starts** - Triple blink pattern

Connect to WiFi: `RallyTelemetry` / `rally2024`

Open browser: http://192.168.4.1/dashboard

## Serial Commands

| Command | Action |
|---------|--------|
| `r` | Start recording |
| `s` | Stop recording |
| `f` | Flush SD card |
| `c` | Calibrate IMU |
| `t` | Task statistics |
| `h` | Help |

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /api/live` | Real-time telemetry JSON |
| `GET /api/files` | List log files |
| `GET /api/convert?file=X.bin` | Download as CSV |
| `GET /download?file=X.bin` | Download binary |

## Data Format

### Binary Log Structure (72 bytes)
```c
struct TelemetryPacket {
    uint32_t magic;        // 'RALL'
    uint16_t version;
    uint16_t sequence;
    uint32_t timestamp_ms;
    
    struct {
        float accel_x, accel_y, accel_z;  // m/s^2
        float gyro_x, gyro_y, gyro_z;     // deg/s
        float temperature;                 // Celsius
    } imu;
    
    struct {
        double latitude, longitude;
        float altitude, speed_kmh, heading;
        uint8_t satellites, fix_quality, hdop;
    } gps;
    
    uint16_t crc16;
} __attribute__((packed));
```

### CSV Export
```csv
Timestamp,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,TempC,Latitude,Longitude,Altitude,SpeedKmh,Heading,Satellites,FixQuality
12345,0.123,-0.456,9.81,1.2,-0.5,0.1,25.4,40.712800,-74.006000,50.0,85.5,180.0,8,1
```

## Configuration

Edit `src/core/config.h`:

```cpp
// Sampling rates
const uint32_t IMU_SAMPLE_RATE_HZ = 100;
const uint32_t GPS_SAMPLE_RATE_HZ = 10;
const uint32_t LOG_RATE_HZ = 50;

// Alert thresholds
const float ALERT_G_FORCE_WARN = 2.5f;
const float ALERT_G_FORCE_CRIT = 3.5f;
const float ALERT_ROLL_WARN = 25.0f;
const float ALERT_ROLL_CRIT = 35.0f;

// WiFi settings
const char* WIFI_AP_SSID = "RallyTelemetry";
const char* WIFI_AP_PASS = "rally2024";
```

## Architecture

```
Core 0 (Real-time)          Core 1 (I/O)
┌─────────────────┐         ┌─────────────────┐
│ Sensor Task     │         │ Logging Task    │
│   - IMU 100Hz   │────┐    │   - SD writes   │
│   - GPS 10Hz    │    │    │   - 50Hz        │
└─────────────────┘    │    └─────────────────┘
         ↓             │             ↑
┌─────────────────┐    │    ┌─────────────────┐
│ Compute Task    │────┘    │ Telemetry Task  │
│   - Data fusion │         │   - UDP stream  │
│   - Alerts      │         │   - Web server  │
└─────────────────┘         └─────────────────┘
```

## Project Structure

```
rallyTelemetry/
├── rallyTelemetry.ino           # Main firmware
├── platformio.ini               # Build config
├── data/dashboard/              # Web UI
│   ├── index.html
│   ├── main.js
│   ├── settings.js
│   └── styles.css
├── src/
│   ├── core/                    # RTOS tasks, state machine
│   ├── sensors/                 # IMU, GPS drivers
│   ├── storage/                 # Binary logger
│   ├── telemetry/               # WiFi, web server
│   ├── alerts/                  # Threshold system
│   └── utils/                   # Ring buffers
└── test/                        # Unit tests
```

## Testing

```bash
# Run all tests
pio test

# Run specific test
pio test -f test_ring_buffer

# Upload and run tests on device
pio test --upload-port /dev/ttyUSB0
```

## Dependencies

- [Adafruit MPU6050](https://github.com/adafruit/Adafruit_MPU6050) ^2.2.4
- [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor) ^1.1.9
- ESP32 Arduino Core (built-in)

## License

MIT License - See [LICENSE](LICENSE)
