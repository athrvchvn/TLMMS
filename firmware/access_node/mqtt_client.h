#pragma once
#include <stdint.h>
#include <stdbool.h>

namespace MqttClient {
    void begin(uint8_t machineId);
    void loop();               // call every iteration for MQTT keepalive
    bool connected();

    // Publish outbound messages
    void publishStatus(const char* stateStr, const char* roll,
                       uint32_t sessionStartTs, int rssi);
    void publishEvent(uint8_t machineId, const char* jsonLine);
    void publishHeartbeat(uint8_t machineId);
    void publishBootEvent(uint8_t machineId);

    // Pending inbound command (polled by state machine)
    bool hasPendingCommand();
    void getPendingCommand(char* outCmd, uint32_t* outTs, char* outIssuedBy);
    void clearPendingCommand();

    // OTA parameters extracted from the last "ota" command payload
    // outUrl must be at least 256 bytes; outVersion at least 32 bytes
    void getOtaParams(char* outUrl, char* outVersion);
}
