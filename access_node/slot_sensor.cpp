#include "slot_sensor.h"
#include "config.h"
#include <Arduino.h>

static bool _stable    = false;  // stable debounced state: true = card present
static bool _raw       = false;
static bool _lastRaw   = false;
static uint32_t _debounceAt = 0;

void SlotSensor::begin() {
    pinMode(PIN_SLOT, INPUT_PULLUP);
    // Read initial state immediately (no debounce on startup)
    _raw = _stable = (digitalRead(PIN_SLOT) == LOW);
    _lastRaw = _raw;
}

void SlotSensor::update() {
    bool raw = (digitalRead(PIN_SLOT) == LOW);

    if (raw != _lastRaw) {
        _lastRaw = raw;
        _debounceAt = millis() + SLOT_DEBOUNCE_MS;
    }

    if (_debounceAt != 0 && millis() >= _debounceAt) {
        _stable = raw;
        _debounceAt = 0;
    }

    _raw = raw;
}

bool SlotSensor::cardPresent() {
    return _stable;
}

bool SlotSensor::cardAbsent() {
    return !_stable;
}
