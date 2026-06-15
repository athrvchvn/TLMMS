#pragma once
#include <stdint.h>
#include <stdbool.h>

namespace SlotSensor {
    void begin();
    bool cardPresent();    // true if card is in slot (debounced)
    bool cardAbsent();     // true if slot is empty (debounced)
    void update();         // call every loop iteration for debounce
}
