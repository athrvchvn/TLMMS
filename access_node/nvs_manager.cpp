#include "nvs_manager.h"
#include "config.h"
#include "secrets.h"
#include <Preferences.h>
#include <string.h>

static Preferences prefs;
static const char* NS = "mms_config";

void NVS::begin() {
    prefs.begin(NS, false);

    // First-boot defaults from config.h / secrets.h
    if (!prefs.isKey("machine_id")) {
        prefs.putUChar("machine_id", MACHINE_ID);
    }
    if (!prefs.isKey("master_key")) {
        prefs.putBytes("master_key", MASTER_KEY, 32);
    }
    if (!prefs.isKey("machine_name")) {
        prefs.putString("machine_name", MACHINE_NAME);
    }
    if (!prefs.isKey("session_limit")) {
        prefs.putUShort("session_limit", SESSION_LIMIT_MIN);
    }
    if (!prefs.isKey("relay_ah")) {
        prefs.putBool("relay_ah", true);
    }
    if (!prefs.isKey("mach_active")) {
        prefs.putBool("mach_active", true);
    }
    if (!prefs.isKey("fw_version")) {
        prefs.putString("fw_version", FW_VERSION);
    }
}

uint8_t NVS::getMachineId() {
    return prefs.getUChar("machine_id", MACHINE_ID);
}

void NVS::setMachineId(uint8_t id) {
    prefs.putUChar("machine_id", id);
}

bool NVS::getMasterKey(uint8_t* out32) {
    return prefs.getBytes("master_key", out32, 32) == 32;
}

void NVS::setMasterKey(const uint8_t* key32) {
    prefs.putBytes("master_key", key32, 32);
}

void NVS::getMachineName(char* out, size_t maxLen) {
    String s = prefs.getString("machine_name", MACHINE_NAME);
    strncpy(out, s.c_str(), maxLen - 1);
    out[maxLen - 1] = '\0';
}

void NVS::setMachineName(const char* name) {
    prefs.putString("machine_name", name);
}

uint16_t NVS::getSessionLimit() {
    return prefs.getUShort("session_limit", SESSION_LIMIT_MIN);
}

void NVS::setSessionLimit(uint16_t minutes) {
    prefs.putUShort("session_limit", minutes);
}

bool NVS::getRelayActiveHigh() {
    return prefs.getBool("relay_ah", true);
}

void NVS::setRelayActiveHigh(bool v) {
    prefs.putBool("relay_ah", v);
}

bool NVS::getMachineActive() {
    return prefs.getBool("mach_active", true);
}

void NVS::setMachineActive(bool v) {
    prefs.putBool("mach_active", v);
}

void NVS::getFwVersion(char* out, size_t maxLen) {
    String s = prefs.getString("fw_version", FW_VERSION);
    strncpy(out, s.c_str(), maxLen - 1);
    out[maxLen - 1] = '\0';
}

void NVS::setFwVersion(const char* v) {
    prefs.putString("fw_version", v);
}
