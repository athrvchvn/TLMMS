#include "relay_controller.h"
#include "config.h"
#include <Arduino.h>

static bool _activeHigh = true;
static bool _on = false;

void Relay::begin(bool activeHigh) {
    _activeHigh = activeHigh;
    pinMode(PIN_RELAY, OUTPUT);
    off();  // Always start safe
}

void Relay::on() {
    _on = true;
    digitalWrite(PIN_RELAY, _activeHigh ? HIGH : LOW);
}

void Relay::off() {
    _on = false;
    digitalWrite(PIN_RELAY, _activeHigh ? LOW : HIGH);
}

bool Relay::isOn() {
    return _on;
}

void Relay::setPolarity(bool activeHigh) {
    bool wasOn = _on;
    _activeHigh = activeHigh;
    // Reapply current state with new polarity
    if (wasOn) on(); else off();
}
