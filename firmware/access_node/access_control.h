#pragma once
#include <stdint.h>
#include <stdbool.h>

namespace AccessControl {
    /**
     * Check if permMask grants access to machineId (1–32).
     */
    bool hasPermission(uint32_t permMask, uint8_t machineId);

    /**
     * Check if uidHex (uppercase hex) is in the revocation list.
     */
    bool isRevoked(const char* uidHex);
}
