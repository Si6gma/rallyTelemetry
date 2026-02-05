# RallyTelemetry

Modular and extensible telemetry system for rally cars using Arduino (ESP32).

## Features

- **Modular Design**: based on a `Sensor` interface, making it easy to add new sensors.
- **Centralized Configuration**: All pins and constants are defined in `config.h`.
- **Generic Storage**: Logs data to SD card in CSV format, independent of the specific sensors used.
- **Sensor Manager**: Automatically handles initialization and data collection for all registered sensors.

## Project Structure

- `src/core`: Core infrastructure (Config, Sensor interface, Manager)
- `src/sensors`: Sensor implementations (GPS, Accelerometer)
- `src/storage`: Storage handling
- `rallyTelemetry.ino`: Main entry point

## Hardware

- ESP32 Development Board
- MPU6050 IMU (Accelerometer + Gyroscope)
- GPS Module (Neo-6M or similar)
- MicroSD Card Module
- Status LED

## Compilation dependencies

- Adafruit Unified Sensor
- Adafruit MPU6050
- SD (built-in)
- SPI (built-in)

## Configuration

Edit `config.h` to change pin assignments or settings:

```cpp
// GPS Settings
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600

// SD Card Settings
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_SCLK_PIN 18
#define SD_CS_PIN 5
```

## How to Add a New Sensor

1.  **Create a new class** that inherits from `Sensor` (in `Sensor.h`).
2.  **Implement the required methods**:
    - `bool begin()`: Initialize hardware.
    - `void update()`: Read data from hardware.
    - `String getHeaderCSV()`: Return column names.
    - `String getDataCSV()`: Return data values.
3.  **Register the sensor** in `rallyTelemetry.ino`:

```cpp
#include "MyNewSensor.h"

MyNewSensor mySensor;

void setup() {
    // ...
    manager.addSensor(&mySensor);
    manager.begin();
    // ...
}
```

## Compilation

Open `rallyTelemetry.ino` in the Arduino IDE or PlatformIO. Ensure you have the required libraries installed:

- Adafruit Unified Sensor
- Adafruit ADXL345
- SD (built-in)
- SPI (built-in)
