#pragma once
#include <stdbool.h>

namespace DisplayStatus {
    void begin();
    void update(bool wifiConnected, bool mqttConnected,
                const char* machineName, const char* timeStr);
}
