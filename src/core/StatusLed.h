#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

class StatusLed {
private:
    int pin;
    unsigned long lastToggleTime;
    bool state;
    int mode; // 0: Off, 1: On, 2: Slow Blink, 3: Fast Blink

    const unsigned long SLOW_BLINK_INTERVAL = 1000;
    const unsigned long FAST_BLINK_INTERVAL = 100;

public:
    StatusLed(int pin);
    void begin();
    void update();
    void setMode(int mode);
    
    static const int MODE_OFF = 0;
    static const int MODE_ON = 1;
    static const int MODE_SLOW_BLINK = 2;
    static const int MODE_FAST_BLINK = 3;
};

#endif // STATUS_LED_H
