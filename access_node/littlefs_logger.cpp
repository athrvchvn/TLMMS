#include "littlefs_logger.h"
#include "config.h"
#include "mqtt_client.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>

static const char* LOG_PATH     = "/logs/sessions.jsonl";
static const char* LOG_OLD_PATH = "/logs/sessions.old.jsonl";

void Logger::begin() {
    if (!LittleFS.begin(true)) {  // true = format on fail
        return;
    }
    LittleFS.mkdir("/logs");
    LittleFS.mkdir("/config");
}

void Logger::appendSessionEnd(const SessionLog& log) {
    // Rotate if file is too large
    if (logFileSize() >= LOG_ROTATE_BYTES) {
        LittleFS.remove(LOG_OLD_PATH);
        LittleFS.rename(LOG_PATH, LOG_OLD_PATH);
    }

    File f = LittleFS.open(LOG_PATH, "a");
    if (!f) return;

    char line[128];
    snprintf(line, sizeof(line),
        "{\"t\":%lu,\"r\":\"%s\",\"m\":%u,\"e\":\"session_end\",\"d\":%lu,\"er\":\"%s\"}\n",
        (unsigned long)log.startTs + log.durationSec,
        log.rollNumber,
        (unsigned)log.machineId,
        (unsigned long)log.durationSec,
        log.endReason
    );
    f.print(line);
    f.flush();
    f.close();
}

void Logger::appendBoot(uint8_t machineId) {
    if (logFileSize() >= LOG_ROTATE_BYTES) {
        LittleFS.remove(LOG_OLD_PATH);
        LittleFS.rename(LOG_PATH, LOG_OLD_PATH);
    }

    File f = LittleFS.open(LOG_PATH, "a");
    if (!f) return;

    char line[80];
    snprintf(line, sizeof(line),
        "{\"t\":%lu,\"m\":%u,\"e\":\"boot\"}\n",
        (unsigned long)time(nullptr), (unsigned)machineId);
    f.print(line);
    f.flush();
    f.close();
}

void Logger::flushToMqtt(uint8_t machineId) {
    if (!LittleFS.exists(LOG_PATH)) {
        // Also flush .old if it exists and was not flushed
        if (!LittleFS.exists(LOG_OLD_PATH)) return;
        LittleFS.rename(LOG_OLD_PATH, LOG_PATH);
    }

    File f = LittleFS.open(LOG_PATH, "r");
    if (!f) return;

    int published = 0, skipped = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        // Validate JSON before publishing
        StaticJsonDocument<200> doc;
        if (deserializeJson(doc, line) != DeserializationError::Ok) {
            skipped++;
            continue;  // partial/corrupt line — discard
        }

        MqttClient::publishEvent(machineId, line.c_str());
        published++;
        yield();  // prevent watchdog trigger during long flushes
    }
    f.close();

    // Delete file after all lines processed
    LittleFS.remove(LOG_PATH);
}

size_t Logger::logFileSize() {
    if (!LittleFS.exists(LOG_PATH)) return 0;
    File f = LittleFS.open(LOG_PATH, "r");
    if (!f) return 0;
    size_t sz = f.size();
    f.close();
    return sz;
}
