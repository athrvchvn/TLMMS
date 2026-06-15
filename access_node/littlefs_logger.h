#pragma once
#include "session_manager.h"

namespace Logger {
    void begin();

    /**
     * Append a session_end event to /logs/sessions.jsonl.
     * Called immediately after Session::end().
     */
    void appendSessionEnd(const SessionLog& log);

    /**
     * Append a boot event (for orphaned session cleanup on RPi bridge).
     */
    void appendBoot(uint8_t machineId);

    /**
     * Flush all valid JSONL lines to MQTT via mqttPublishEvent().
     * Skips malformed (partial-write) lines. Deletes file after all lines published.
     * Call after MQTT connects.
     */
    void flushToMqtt(uint8_t machineId);

    /**
     * Return approximate file size in bytes (0 if no log file).
     */
    size_t logFileSize();
}
