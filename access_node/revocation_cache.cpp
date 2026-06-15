#include "revocation_cache.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <string.h>

static const char* REVOKED_PATH = "/config/revoked.json";
static const int   MAX_REVOKED  = 64;

// In-memory cache (loaded on begin() and on update)
static char _uids[MAX_REVOKED][9];  // 8 hex chars + null
static int  _count = 0;

static void loadFromFile() {
    _count = 0;
    if (!LittleFS.exists(REVOKED_PATH)) return;

    File f = LittleFS.open(REVOKED_PATH, "r");
    if (!f) return;

    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, f) != DeserializationError::Ok) {
        f.close();
        return;
    }
    f.close();

    JsonArray arr = doc["uids"].as<JsonArray>();
    for (JsonVariant v : arr) {
        if (_count >= MAX_REVOKED) break;
        const char* uid = v.as<const char*>();
        if (uid) {
            strncpy(_uids[_count], uid, 8);
            _uids[_count][8] = '\0';
            _count++;
        }
    }
}

void Revocation::begin() {
    loadFromFile();
}

bool Revocation::isRevoked(const char* uidHex) {
    for (int i = 0; i < _count; i++) {
        if (strcasecmp(_uids[i], uidHex) == 0) return true;
    }
    return false;
}

void Revocation::updateFromJson(const char* jsonPayload) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, jsonPayload) != DeserializationError::Ok) return;

    // Persist to LittleFS
    File f = LittleFS.open(REVOKED_PATH, "w");
    if (f) {
        f.print(jsonPayload);
        f.flush();
        f.close();
    }

    // Reload in-memory cache
    loadFromFile();
}
