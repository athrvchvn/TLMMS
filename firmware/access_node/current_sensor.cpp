#include "current_sensor.h"
#include "config.h"
#include <Arduino.h>

// Raw 12-bit ADC sample captured each loop iteration.
// Future: calibration, offset compensation, filtering, and amp conversion go here.
static uint16_t _rawAdc = 0;

void CurrentSensor::begin() {
    // GPIO 36 is input-only (ADC1 CH0); explicit INPUT mode documents intent.
    // No internal pull-up is available on input-only pins — none is needed for analog.
    pinMode(PIN_CURRENT_SENSOR, INPUT);
}

void CurrentSensor::update() {
    _rawAdc = static_cast<uint16_t>(analogRead(PIN_CURRENT_SENSOR));
}

uint16_t CurrentSensor::getRaw() {
    return _rawAdc;
}
