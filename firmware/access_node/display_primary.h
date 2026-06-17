#pragma once
#include <stdint.h>

enum class DisplayState {
    IDLE,
    CARD_READING,
    ACCESS_DENIED,
    SESSION_ACTIVE,
    TIMEOUT_WARNING,
    SESSION_ENDED,
    MACHINE_DISABLED,
    OTA_UPDATE,
    ERROR,
};

namespace DisplayPrimary {
    void begin();
    void showIdle(const char* machineName);
    void showCardReading();
    void showAccessDenied(const char* reason, const char* roll);
    void showSessionActive(const char* roll, const char* machineName,
                           uint32_t elapsedSec, uint32_t limitSec);
    void showTimeoutWarning(const char* roll, uint32_t remainingSec);
    void showSessionEnded(const char* roll, uint32_t durationSec);
    void showMachineDisabled();
    void showOtaUpdate(const char* oldVer, const char* newVer, int progressPct);
    void showError(const char* code);
    void update();  // call in loop for animations (spinner, etc.)
}
