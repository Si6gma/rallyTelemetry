#include "StatusLed.h"

StatusLed::StatusLed(int pin) {
    this->pin = pin;
    this->mode = MODE_OFF;
    this->state = false;
    this->lastToggleTime = 0;
}

void StatusLed::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void StatusLed::setMode(int mode) {
    this->mode = mode;
    if (mode == MODE_ON) {
        digitalWrite(pin, HIGH);
    } else if (mode == MODE_OFF) {
        digitalWrite(pin, LOW);
    }
}

void StatusLed::update() {
    if (mode == MODE_SLOW_BLINK || mode == MODE_FAST_BLINK) {
        unsigned long interval = (mode == MODE_SLOW_BLINK) ? SLOW_BLINK_INTERVAL : FAST_BLINK_INTERVAL;
        if (millis() - lastToggleTime >= interval) {
            state = !state;
            digitalWrite(pin, state ? HIGH : LOW);
            lastToggleTime = millis();
        }
    }
}
