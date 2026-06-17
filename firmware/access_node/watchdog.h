#pragma once

namespace Watchdog {
    void begin();   // Initialize TWDT with WATCHDOG_TIMEOUT_SEC
    void feed();    // Reset watchdog timer — call every main loop iteration
}
