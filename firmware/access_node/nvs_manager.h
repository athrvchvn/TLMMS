#pragma once
#include <stdint.h>

namespace NVS {
    void begin();

    uint8_t  getMachineId();
    void     setMachineId(uint8_t id);

    bool     getMasterKey(uint8_t* outKey32);
    void     setMasterKey(const uint8_t* key32);

    void     getMachineName(char* out, size_t maxLen);
    void     setMachineName(const char* name);

    uint16_t getSessionLimit();
    void     setSessionLimit(uint16_t minutes);

    bool     getRelayActiveHigh();
    void     setRelayActiveHigh(bool active);

    bool     getMachineActive();
    void     setMachineActive(bool active);

    void     getFwVersion(char* out, size_t maxLen);
    void     setFwVersion(const char* version);
}
