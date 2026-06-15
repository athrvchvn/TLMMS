#include "access_control.h"
#include "revocation_cache.h"

bool AccessControl::hasPermission(uint32_t permMask, uint8_t machineId) {
    if (machineId < 1 || machineId > 32) return false;
    return (permMask >> (machineId - 1)) & 0x01;
}

bool AccessControl::isRevoked(const char* uidHex) {
    return Revocation::isRevoked(uidHex);
}
