#pragma once
#include <stdbool.h>

namespace OTA {
    /**
     * Download and apply firmware from url using esp_https_ota.
     * Transitions display to OTA state, disables relay (safety).
     * Returns false if download or verification fails.
     * On success: saves new version to NVS and restarts.
     */
    bool applyUpdate(const char* url, const char* newVersion);
}
