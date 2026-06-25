#pragma once
#include <cstdint>

// Current sensor on GPIO 36 (ADC1 CH0 / SVP — input-only pin).
// Call begin() in setup(), update() every loop via StateMachine::tick().
// getRaw() returns the latest 12-bit ADC sample (0–4095).
//
// Future work: calibration, filtering, amp conversion, and safety thresholds
// all go in current_sensor.cpp (or a new current_processor.cpp that calls getRaw()).

namespace CurrentSensor {
    void     begin();   // Configure ADC pin
    void     update();  // Sample ADC — called every loop iteration
    uint16_t getRaw();  // Raw 12-bit count (0–4095); no processing applied
}
