#pragma once
#include <stdbool.h>

namespace Relay {
    void begin(bool activeHigh = true);
    void on();
    void off();
    bool isOn();
    void setPolarity(bool activeHigh);  // called when MQTT config updates relay_active_high
}
