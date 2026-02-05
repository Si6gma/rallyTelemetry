#include "src/core/config.h"
#include "src/core/SensorManager.h"
#include "src/core/StatusLed.h"
#include "src/sensors/imu.h"
#include "src/sensors/gps.h"
#include "src/storage/storage.h"

SensorManager manager;
Storage storage;
StatusLed statusLed(STATUS_LED_PIN);
IMU imuSensor;
GPS gpsSensor;

void setup() {
    Serial.begin(SERIAL_BAUD);
    statusLed.begin();
    statusLed.setMode(StatusLed::MODE_ON); // Booting

    // Initialize Storage
    Serial.println("Initializing Storage...");
    if (!storage.begin()) {
        Serial.println("Storage Initialization Failed!");
        statusLed.setMode(StatusLed::MODE_FAST_BLINK); // Error
        while(1); // Halt
    } else {
        Serial.println("Storage Initialized.");
        storage.setFilename("/log.csv");
    }

    // Initialize Sensors
    Serial.println("Initializing Sensors...");
    manager.addSensor(&imuSensor);
    manager.addSensor(&gpsSensor);
    
    manager.begin();
    
    // Write Header if file is new
    if (!storage.fileExists("/log.csv")) {
        Print* file = storage.getPrintInterface();
        if (file) {
             manager.printCombinedHeader(*file);
             file->println();
             storage.flush();
        }
    }
    
    Serial.println("Setup Complete.");
    statusLed.setMode(StatusLed::MODE_SLOW_BLINK); // Recording
}

void loop() {
    statusLed.update();
    
    // Update all sensors
    manager.update();

    // Log data (you might want to add a timer here to limit logging rate)
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 100) { // Log every 100ms
        
        Print* file = storage.getPrintInterface();
        if (file) {
            manager.printCombinedData(*file);
            file->println();
            
            // Periodically flush to ensure data is saved
            static int writeCount = 0;
            if (++writeCount > 10) { // Flush every 10 lines (1 second approx)
                storage.flush();
                writeCount = 0;
            }
        }
        
        lastLogTime = millis();
    }
}