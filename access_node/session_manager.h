#pragma once
#include <stdint.h>
#include <stdbool.h>

struct SessionLog {
    uint32_t startTs;       // Unix timestamp
    uint32_t durationSec;
    char     rollNumber[10];
    uint8_t  machineId;
    char     endReason[16]; // "card_removed" | "timeout" | "remote_stop" | "power_loss"
};

namespace Session {
    void    begin();
    void    start(const char* roll, uint8_t machineId);
    void    end(const char* reason);
    bool    isActive();
    uint32_t elapsedSeconds();
    uint32_t remainingSeconds(uint16_t limitMinutes);
    bool    isWarning(uint16_t limitMinutes);
    bool    isExpired(uint16_t limitMinutes);
    void    getLog(SessionLog* out);  // fills log struct for the just-ended session
    const char* getRoll();
    uint32_t startTimestamp();
}
