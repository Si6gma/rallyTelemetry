#ifndef CONFIG_H
#define CONFIG_H

// System Settings
#define SERIAL_BAUD 115200

// GPS Settings
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600
#define GPS_BUFFER_SIZE 256

// SD Card Settings
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_SCLK_PIN 18
#define SD_CS_PIN 5

// Status LED
#define STATUS_LED_PIN 2

// Physical Constants
#define GRAVITY_CONSTANT 9.80665

#endif // CONFIG_H
