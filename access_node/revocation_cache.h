#pragma once
#include <stdint.h>
#include <stdbool.h>

namespace Revocation {
    void begin();

    /**
     * Check if a card UID (uppercase hex string, e.g. "A1B2C3D4") is revoked.
     * Returns true if the UID is in the revocation list → deny access.
     */
    bool isRevoked(const char* uidHex);

    /**
     * Update the revocation list from a JSON payload.
     * Expected format: {"uids":["A1B2C3","..."], "updated_at":unix_ts}
     * Persists to LittleFS /config/revoked.json
     */
    void updateFromJson(const char* jsonPayload);
}
