<!--
Suggested GitHub Topics:
arduino, esp32, telemetry, rally, racing, gps, imu, embedded-systems, cpp, motorsports, data-logging, sd-card, mpu6050, neo-6m
-->

# RallyTelemetry

A modular, real-time telemetry data logger for rally cars built on ESP32. Captures GPS position and IMU (accelerometer/gyroscope) data at 10Hz to SD card for post-race analysis.

## Why It Exists

Rally driving generates extreme G-forces and rapid direction changes that are hard to analyze subjectively. This system provides objective data to:
- **Analyze driving lines** through GPS traces
- **Measure cornering forces** and braking intensity via accelerometer
- **Identify setup improvements** by correlating handling data with lap times
- **Learn from data** rather than relying solely on driver feel

## Tech Stack

| Component | Technology |
|-----------|------------|
| Platform | ESP32 (Arduino framework) |
| IMU | MPU6050 (via Adafruit library) |
| GPS | Neo-6M NMEA parser (custom implementation) |
| Storage | MicroSD via SPI |
| Language | C++17 |
| Build Tool | Arduino IDE or PlatformIO |

## Hardware Requirements

- **ESP32 Development Board** (DevKit v1 or similar)
- **MPU6050** 6-axis accelerometer/gyroscope (I2C)
- **Neo-6M GPS Module** (UART)
- **MicroSD Card Module** (SPI)
- **Status LED** (GPIO 2, onboard OK)
- **Power**: 5V USB or 7-12V via VIN

### Wiring Diagram

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| GPS RX | GPIO 16 | UART2 RX |
| GPS TX | GPIO 17 | UART2 TX |
| SD MOSI | GPIO 23 | SPI MOSI |
| SD MISO | GPIO 19 | SPI MISO |
| SD SCK | GPIO 18 | SPI Clock |
| SD CS | GPIO 5 | Chip Select |
| MPU6050 SDA | GPIO 21 | I2C Default |
| MPU6050 SCL | GPIO 22 | I2C Default |

## How to Build/Run

### Arduino IDE

1. Install required libraries via Library Manager:
   - `Adafruit MPU6050`
   - `Adafruit Unified Sensor`
   - `SD` (built-in)
   - `SPI` (built-in)

2. Open `rallyTelemetry.ino` in Arduino IDE

3. Select board: **ESP32 Dev Module**

4. Compile and upload to ESP32

### PlatformIO

```bash
# Create platformio.ini in project root:
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    adafruit/Adafruit MPU6050 @ ^2.2.4
    adafruit/Adafruit Unified Sensor @ ^1.1.9
```

## Project Structure

```
rallyTelemetry/
├── rallyTelemetry.ino      # Main entry point
├── src/
│   ├── core/
│   │   ├── Sensor.h          # Abstract Sensor interface
│   │   ├── SensorManager.h   # Manages sensor lifecycle
│   │   ├── StatusLed.h       # Visual feedback system
│   │   └── config.h          # Pin assignments & constants
│   ├── sensors/
│   │   ├── imu.h/.cpp        # MPU6050 implementation
│   │   └── gps.h/.cpp        # NMEA GPS parser
│   └── storage/
│       └── storage.h/.cpp    # SD card CSV logging
└── README.md
```

## Output Format

Data is logged to `/log.csv` on the SD card:

```csv
Timestamp,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,TempC,GForce,GPS_Time,Latitude,Longitude,SatCount,FixQuality
12345,0.12,-0.05,9.81,0.01,-0.02,0.00,24.5,1.00,123519,40.123456,-74.654321,8,1
```

## Key Learnings

- **OOP in Embedded**: Applied polymorphism via abstract `Sensor` interface—enables swapping sensors without changing core logic. Trade-off: vtable overhead acceptable for ESP32's resources.

- **Memory-Conscious Design**: Used `Print&` references and avoided `String` concatenation in hot paths. Prevents heap fragmentation during long logging sessions.

- **NMEA Parsing**: Implemented custom GPS parser instead of library—reduced flash usage by ~15KB and allowed 10Hz update rate tuning via UBX commands.

- **Real-Time Constraints**: 100ms logging loop with periodic `flush()` balances data integrity (crash protection) with SD card write endurance.

## Adding New Sensors

1. Create class inheriting from `Sensor`:
```cpp
class MySensor : public Sensor {
    bool begin() override;
    void update() override;
    void printHeaderCSV(Print& p) override;
    void printDataCSV(Print& p) override;
};
```

2. Register in `rallyTelemetry.ino`:
```cpp
MySensor mySensor;
manager.addSensor(&mySensor);
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

*Built for the love of rally and clean embedded code.*
