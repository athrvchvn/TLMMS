#include "mqtt_client.h"
#include "config.h"
#include "secrets.h"
#include "nvs_manager.h"
#include "revocation_cache.h"
#include "relay_controller.h"
#include "littlefs_logger.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

static WiFiClient   _wifiClient;
static PubSubClient _mqttClient(_wifiClient);

static uint8_t  _machineId = 0;
static uint32_t _lastReconnectMs = 0;
static uint32_t _lastHeartbeatMs = 0;

// Pending command from MQTT subscription
static bool     _hasPendingCmd = false;
static char     _pendingCmd[16];
static uint32_t _pendingTs = 0;
static char     _pendingIssuedBy[64];

// OTA-specific fields — populated when _pendingCmd == "ota"
static char _pendingOtaUrl[256];
static char _pendingOtaVersion[32];

// ---------------------------------------------------------------------------
// Topic builder helpers
// ---------------------------------------------------------------------------
static void topicStatus(char* out)    { snprintf(out, 64, "mms/nodes/%u/status",    _machineId); }
static void topicEvent(char* out)     { snprintf(out, 64, "mms/nodes/%u/event",     _machineId); }
static void topicHeartbeat(char* out) { snprintf(out, 64, "mms/nodes/%u/heartbeat", _machineId); }
static void topicCommand(char* out)   { snprintf(out, 64, "mms/nodes/%u/command",   _machineId); }
static void topicConfig(char* out)    { snprintf(out, 64, "mms/nodes/%u/config",    _machineId); }

// ---------------------------------------------------------------------------
// MQTT callback
// ---------------------------------------------------------------------------
static void onMessage(char* topic, byte* payload, unsigned int length) {
    char buf[512];
    size_t len = (length < sizeof(buf) - 1) ? length : sizeof(buf) - 1;
    memcpy(buf, payload, len);
    buf[len] = '\0';

    char cmdTopic[64], cfgTopic[64], revokedTopic[] = "mms/all/revoked";
    topicCommand(cmdTopic);
    topicConfig(cfgTopic);

    if (strcmp(topic, cmdTopic) == 0) {
        // Inbound command — buffer increased to 512 to accommodate OTA URL
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, buf) == DeserializationError::Ok) {
            const char* cmd = doc["cmd"]       | "";
            uint32_t ts     = doc["ts"]        | 0;
            const char* by  = doc["issued_by"] | "";
            strncpy(_pendingCmd,      cmd, sizeof(_pendingCmd)      - 1);
            strncpy(_pendingIssuedBy, by,  sizeof(_pendingIssuedBy) - 1);
            _pendingTs = ts;

            // Extract OTA-specific fields so state_machine can pass them to OTA::applyUpdate()
            if (strcmp(cmd, "ota") == 0) {
                strncpy(_pendingOtaUrl,     doc["url"]     | "", sizeof(_pendingOtaUrl)     - 1);
                strncpy(_pendingOtaVersion, doc["version"] | "", sizeof(_pendingOtaVersion) - 1);
            }

            _hasPendingCmd = true;
        }

    } else if (strcmp(topic, cfgTopic) == 0) {
        // Config update
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, buf) == DeserializationError::Ok) {
            if (doc.containsKey("name")) {
                NVS::setMachineName(doc["name"] | "");
            }
            if (doc.containsKey("session_limit_min")) {
                NVS::setSessionLimit(doc["session_limit_min"] | SESSION_LIMIT_MIN);
            }
            if (doc.containsKey("relay_active_high")) {
                bool rah = doc["relay_active_high"] | true;
                NVS::setRelayActiveHigh(rah);
                Relay::setPolarity(rah);
            }
            if (doc.containsKey("machine_active")) {
                NVS::setMachineActive(doc["machine_active"] | true);
            }
        }

    } else if (strcmp(topic, revokedTopic) == 0) {
        Revocation::updateFromJson(buf);
    }
}

// ---------------------------------------------------------------------------
// Reconnect
// ---------------------------------------------------------------------------
static bool reconnect() {
    if (_mqttClient.connected()) return true;

    char clientId[32];
    snprintf(clientId, sizeof(clientId), "mms_node_%u", _machineId);

    // Last Will: offline status (retained)
    char statusTopic[64];
    topicStatus(statusTopic);
    const char* lwt = "{\"state\":\"offline\"}";

    if (_mqttClient.connect(clientId, MQTT_USER, MQTT_PASSWORD,
                            statusTopic, 1, true, lwt)) {
        // Subscribe to inbound topics
        char cmdTopic[64], cfgTopic[64];
        topicCommand(cmdTopic);
        topicConfig(cfgTopic);
        _mqttClient.subscribe(cmdTopic,         1);
        _mqttClient.subscribe(cfgTopic,         1);
        _mqttClient.subscribe("mms/all/revoked",1);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
void MqttClient::begin(uint8_t machineId) {
    _machineId = machineId;
    _mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    _mqttClient.setCallback(onMessage);
    _mqttClient.setKeepAlive(60);
    _mqttClient.setBufferSize(1536);  // large enough for OTA command with Firebase Storage URL
    reconnect();
}

void MqttClient::loop() {
    if (!_mqttClient.connected()) {
        uint32_t now = millis();
        if (now - _lastReconnectMs >= MQTT_RECONNECT_DELAY_MS) {
            _lastReconnectMs = now;
            if (reconnect()) {
                // On reconnect: flush cached logs
                Logger::flushToMqtt(_machineId);
            }
        }
    }
    _mqttClient.loop();

    // Periodic heartbeat
    uint32_t now = millis();
    if (now - _lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        _lastHeartbeatMs = now;
        publishHeartbeat(_machineId);
    }
}

bool MqttClient::connected() {
    return _mqttClient.connected();
}

void MqttClient::publishStatus(const char* stateStr, const char* roll,
                                uint32_t sessionStartTs, int rssi) {
    if (!_mqttClient.connected()) return;

    char topic[64];
    topicStatus(topic);

    char fwVer[16];
    NVS::getFwVersion(fwVer, sizeof(fwVer));

    char buf[256];
    if (roll && roll[0]) {
        snprintf(buf, sizeof(buf),
            "{\"state\":\"%s\",\"roll\":\"%s\",\"session_start\":%lu,"
            "\"fw_version\":\"%s\",\"ip\":\"%s\",\"ts\":%lu,\"rssi\":%d}",
            stateStr, roll, (unsigned long)sessionStartTs,
            fwVer, WiFi.localIP().toString().c_str(),
            (unsigned long)time(nullptr), rssi);
    } else {
        snprintf(buf, sizeof(buf),
            "{\"state\":\"%s\",\"roll\":null,\"session_start\":null,"
            "\"fw_version\":\"%s\",\"ip\":\"%s\",\"ts\":%lu,\"rssi\":%d}",
            stateStr, fwVer, WiFi.localIP().toString().c_str(),
            (unsigned long)time(nullptr), rssi);
    }

    _mqttClient.publish(topic, (const uint8_t*)buf, strlen(buf), true);  // retained
}

void MqttClient::publishEvent(uint8_t machineId, const char* jsonLine) {
    if (!_mqttClient.connected()) return;
    char topic[64];
    snprintf(topic, sizeof(topic), "mms/nodes/%u/event", machineId);
    _mqttClient.publish(topic, jsonLine, false);
}

void MqttClient::publishHeartbeat(uint8_t machineId) {
    if (!_mqttClient.connected()) return;
    char topic[64], buf[128];
    snprintf(topic, sizeof(topic), "mms/nodes/%u/heartbeat", machineId);
    snprintf(buf,   sizeof(buf),
        "{\"ts\":%lu,\"heap_free\":%lu,\"uptime_s\":%lu}",
        (unsigned long)time(nullptr),
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)millis() / 1000);
    _mqttClient.publish(topic, buf, false);
}

void MqttClient::publishBootEvent(uint8_t machineId) {
    if (!_mqttClient.connected()) return;
    char topic[64], buf[80];
    snprintf(topic, sizeof(topic), "mms/nodes/%u/event", machineId);
    snprintf(buf,   sizeof(buf),
        "{\"t\":%lu,\"m\":%u,\"e\":\"boot\"}",
        (unsigned long)time(nullptr), (unsigned)machineId);
    _mqttClient.publish(topic, buf, false);
}

bool MqttClient::hasPendingCommand() {
    return _hasPendingCmd;
}

void MqttClient::getPendingCommand(char* outCmd, uint32_t* outTs, char* outIssuedBy) {
    strncpy(outCmd,       _pendingCmd,       15);
    strncpy(outIssuedBy, _pendingIssuedBy,  63);
    *outTs = _pendingTs;
}

void MqttClient::clearPendingCommand() {
    _hasPendingCmd = false;
    _pendingCmd[0]      = '\0';
    _pendingTs          = 0;
    _pendingIssuedBy[0] = '\0';
    _pendingOtaUrl[0]     = '\0';
    _pendingOtaVersion[0] = '\0';
}

void MqttClient::getOtaParams(char* outUrl, char* outVersion) {
    strncpy(outUrl,     _pendingOtaUrl,     255); outUrl[255]     = '\0';
    strncpy(outVersion, _pendingOtaVersion,  31); outVersion[31]  = '\0';
}
