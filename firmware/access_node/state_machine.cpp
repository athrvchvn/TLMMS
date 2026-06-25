#include "state_machine.h"
#include "config.h"
#include "nvs_manager.h"
#include "slot_sensor.h"
#include "current_sensor.h"
#include "rfid_handler.h"
#include "card_schema.h"
#include "access_control.h"
#include "relay_controller.h"
#include "session_manager.h"
#include "mqtt_client.h"
#include "littlefs_logger.h"
#include "display_primary.h"
#include "display_status.h"
#include "ota_handler.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <string.h>

static State    _state          = State::BOOT;
static uint32_t _stateEnteredMs = 0;
static char     _machineName[33];
static uint8_t  _machineId      = 0;
static uint8_t  _masterKey[32];

// Denial reason shown on OLED
static char _denialReason[32];

// OTA pending command details
static char     _otaUrl[256];
static char     _otaVersion[16];
static bool     _otaPending     = false;

static void enterState(State next) {
    _state          = next;
    _stateEnteredMs = millis();
}

static uint32_t stateElapsedMs() {
    return millis() - _stateEnteredMs;
}

// ---------------------------------------------------------------------------
// Status strip update helper
// ---------------------------------------------------------------------------
static void updateStatusStrip() {
    char timeStr[8] = "OFFLINE";
    struct tm ti;
    time_t now = time(nullptr);
    if (now > 1700000000UL && localtime_r(&now, &ti)) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", ti.tm_hour, ti.tm_min);
    }
    DisplayStatus::update(
        WiFi.status() == WL_CONNECTED,
        MqttClient::connected(),
        _machineName,
        timeStr
    );
}

// ---------------------------------------------------------------------------
// Process pending MQTT command
// Returns true if command was handled (state changed or action taken)
// ---------------------------------------------------------------------------
static bool processPendingCommand() {
    if (!MqttClient::hasPendingCommand()) return false;

    char cmd[16], issuedBy[64];
    uint32_t ts;
    MqttClient::getPendingCommand(cmd, &ts, issuedBy);
    MqttClient::clearPendingCommand();

    // Freshness check: 5-minute TTL
    uint32_t now = (uint32_t)time(nullptr);
    if (now > 1700000000UL && ts > 0) {
        uint32_t age = now - ts;
        if (age > 300) {
            return false;  // Stale command — discard
        }
    }

    if (strcmp(cmd, "stop") == 0) {
        if (_state == State::SESSION_ACTIVE || _state == State::TIMEOUT_WARNING) {
            Relay::off();
            Session::end("remote_stop");
            SessionLog log;
            Session::getLog(&log);
            Logger::appendSessionEnd(log);
            char eventJson[256];
            snprintf(eventJson, sizeof(eventJson),
                "{\"t\":%lu,\"r\":\"%s\",\"m\":%u,\"e\":\"session_end\",\"d\":%lu,\"er\":\"remote_stop\"}",
                (unsigned long)log.startTs + log.durationSec,
                log.rollNumber, (unsigned)log.machineId,
                (unsigned long)log.durationSec);
            MqttClient::publishEvent(_machineId, eventJson);
            MqttClient::publishStatus("idle", nullptr, 0, WiFi.RSSI());
            DisplayPrimary::showSessionEnded(log.rollNumber, log.durationSec);
            enterState(State::SESSION_ENDED);
            return true;
        }
    } else if (strcmp(cmd, "ota") == 0) {
        // Fetch URL and version before clearing the pending command
        MqttClient::getOtaParams(_otaUrl, _otaVersion);
        if (_otaUrl[0] == '\0') {
            // Bridge sent ota command with no URL — nothing to do
            return false;
        }
        _otaPending = true;
        return false;  // Let tick() handle the state transition
    } else if (strcmp(cmd, "reboot") == 0) {
        ESP.restart();
    }
    return false;
}

// ---------------------------------------------------------------------------
// StateMachine::begin
// ---------------------------------------------------------------------------
void StateMachine::begin() {
    _machineId = NVS::getMachineId();
    NVS::getMasterKey(_masterKey);
    NVS::getMachineName(_machineName, sizeof(_machineName));

    // Check if card was in slot before reset (boot with card present)
    SlotSensor::update();
    if (SlotSensor::cardPresent()) {
        enterState(State::CARD_READING);
    } else {
        enterState(State::IDLE);
    }
}

// ---------------------------------------------------------------------------
// StateMachine::tick — called every loop()
// ---------------------------------------------------------------------------
void StateMachine::tick() {
    SlotSensor::update();
    CurrentSensor::update();
    MqttClient::loop();
    DisplayPrimary::update();

    // Refresh NVS config values
    NVS::getMachineName(_machineName, sizeof(_machineName));
    uint16_t limitMin = NVS::getSessionLimit();

    // OTA takes priority over everything
    if (_otaPending) {
        _otaPending = false;
        // OTA::applyUpdate() never returns on success; on failure returns false
        OTA::applyUpdate(_otaUrl, _otaVersion);
        // If failed, continue normal operation
    }

    // Check for pending MQTT commands in all states
    processPendingCommand();

    // Machine disabled check (applies in all non-active states)
    if (!NVS::getMachineActive() && _state != State::SESSION_ACTIVE &&
        _state != State::TIMEOUT_WARNING && _state != State::OTA_UPDATE) {
        if (_state != State::MACHINE_DISABLED) {
            Relay::off();
            DisplayPrimary::showMachineDisabled();
            MqttClient::publishStatus("disabled", nullptr, 0, WiFi.RSSI());
            enterState(State::MACHINE_DISABLED);
        }
        updateStatusStrip();
        return;
    }

    switch (_state) {

    // -----------------------------------------------------------------------
    case State::IDLE:
        if (SlotSensor::cardPresent()) {
            DisplayPrimary::showCardReading();
            enterState(State::CARD_READING);
        } else {
            if (stateElapsedMs() > 500) {
                DisplayPrimary::showIdle(_machineName);
                _stateEnteredMs = millis() - 600;  // Rate-limit idle redraws
            }
        }
        break;

    // -----------------------------------------------------------------------
    case State::CARD_READING: {
        if (!SlotSensor::cardPresent()) {
            // Card removed before read completed
            enterState(State::IDLE);
            break;
        }

        CardData card;
        bool readOk = CardSchema::readCard(_masterKey, &card);

        if (!readOk || !card.valid) {
            strncpy(_denialReason, "Unreadable card", sizeof(_denialReason));
            DisplayPrimary::showAccessDenied(_denialReason, "---");
            MqttClient::publishStatus("denied", nullptr, 0, WiFi.RSSI());
            enterState(State::ACCESS_DENIED);
            break;
        }

        if (card.schemaVersion != CARD_SCHEMA_VERSION) {
            strncpy(_denialReason, "Old card format", sizeof(_denialReason));
            DisplayPrimary::showAccessDenied(_denialReason, card.rollNumber);
            enterState(State::ACCESS_DENIED);
            break;
        }

        // Build UID hex string for revocation check
        extern MFRC522 rfid;
        char uidHex[9];
        snprintf(uidHex, sizeof(uidHex), "%02X%02X%02X%02X",
            rfid.uid.uidByte[0], rfid.uid.uidByte[1],
            rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

        if (AccessControl::isRevoked(uidHex)) {
            strncpy(_denialReason, "Card revoked", sizeof(_denialReason));
            DisplayPrimary::showAccessDenied(_denialReason, card.rollNumber);
            enterState(State::ACCESS_DENIED);
            break;
        }

        if (!AccessControl::hasPermission(card.permMask, _machineId)) {
            strncpy(_denialReason, "No permission", sizeof(_denialReason));
            DisplayPrimary::showAccessDenied(_denialReason, card.rollNumber);
            MqttClient::publishStatus("denied", card.rollNumber, 0, WiFi.RSSI());
            enterState(State::ACCESS_DENIED);
            break;
        }

        // ACCESS GRANTED
        Relay::on();
        Session::start(card.rollNumber, _machineId);
        DisplayPrimary::showSessionActive(card.rollNumber, _machineName, 0, limitMin * 60);
        MqttClient::publishStatus("active", card.rollNumber, Session::startTimestamp(), WiFi.RSSI());
        enterState(State::SESSION_ACTIVE);
        break;
    }

    // -----------------------------------------------------------------------
    case State::ACCESS_DENIED:
        // Auto-clear after 3 seconds
        if (stateElapsedMs() >= 3000) {
            DisplayPrimary::showIdle(_machineName);
            MqttClient::publishStatus("idle", nullptr, 0, WiFi.RSSI());
            enterState(State::IDLE);
        }
        break;

    // -----------------------------------------------------------------------
    case State::SESSION_ACTIVE:
        if (!SlotSensor::cardPresent()) {
            // Card removed — end session
            Relay::off();
            Session::end("card_removed");
            SessionLog log;
            Session::getLog(&log);
            Logger::appendSessionEnd(log);

            char eventJson[256];
            snprintf(eventJson, sizeof(eventJson),
                "{\"t\":%lu,\"r\":\"%s\",\"m\":%u,\"e\":\"session_end\",\"d\":%lu,\"er\":\"card_removed\"}",
                (unsigned long)log.startTs + log.durationSec,
                log.rollNumber, (unsigned)log.machineId,
                (unsigned long)log.durationSec);
            MqttClient::publishEvent(_machineId, eventJson);
            MqttClient::publishStatus("idle", nullptr, 0, WiFi.RSSI());
            DisplayPrimary::showSessionEnded(log.rollNumber, log.durationSec);
            enterState(State::SESSION_ENDED);
            break;
        }

        if (Session::isExpired(limitMin)) {
            enterState(State::TIMEOUT_EXPIRED);
            break;
        }

        if (Session::isWarning(limitMin)) {
            DisplayPrimary::showTimeoutWarning(Session::getRoll(), Session::remainingSeconds(limitMin));
            enterState(State::TIMEOUT_WARNING);
            break;
        }

        // Update active display every second
        if (stateElapsedMs() % 1000 < 50) {
            DisplayPrimary::showSessionActive(Session::getRoll(), _machineName,
                Session::elapsedSeconds(), limitMin * 60);
        }
        break;

    // -----------------------------------------------------------------------
    case State::TIMEOUT_WARNING:
        if (!SlotSensor::cardPresent()) {
            // Treat same as card removed
            Relay::off();
            Session::end("card_removed");
            SessionLog log;
            Session::getLog(&log);
            Logger::appendSessionEnd(log);
            char eventJson[256];
            snprintf(eventJson, sizeof(eventJson),
                "{\"t\":%lu,\"r\":\"%s\",\"m\":%u,\"e\":\"session_end\",\"d\":%lu,\"er\":\"card_removed\"}",
                (unsigned long)log.startTs + log.durationSec,
                log.rollNumber, (unsigned)log.machineId,
                (unsigned long)log.durationSec);
            MqttClient::publishEvent(_machineId, eventJson);
            MqttClient::publishStatus("idle", nullptr, 0, WiFi.RSSI());
            DisplayPrimary::showSessionEnded(log.rollNumber, log.durationSec);
            enterState(State::SESSION_ENDED);
            break;
        }

        if (Session::isExpired(limitMin)) {
            enterState(State::TIMEOUT_EXPIRED);
        } else {
            DisplayPrimary::showTimeoutWarning(Session::getRoll(), Session::remainingSeconds(limitMin));
        }
        break;

    // -----------------------------------------------------------------------
    case State::TIMEOUT_EXPIRED:
        Relay::off();
        Session::end("timeout");
        {
            SessionLog log;
            Session::getLog(&log);
            Logger::appendSessionEnd(log);
            char eventJson[256];
            snprintf(eventJson, sizeof(eventJson),
                "{\"t\":%lu,\"r\":\"%s\",\"m\":%u,\"e\":\"session_end\",\"d\":%lu,\"er\":\"timeout\"}",
                (unsigned long)log.startTs + log.durationSec,
                log.rollNumber, (unsigned)log.machineId,
                (unsigned long)log.durationSec);
            MqttClient::publishEvent(_machineId, eventJson);
            MqttClient::publishStatus("idle", nullptr, 0, WiFi.RSSI());
            DisplayPrimary::showSessionEnded(log.rollNumber, log.durationSec);
        }
        enterState(State::SESSION_ENDED);
        break;

    // -----------------------------------------------------------------------
    case State::SESSION_ENDED:
        // Show "thank you" screen for 3 seconds then return to IDLE
        if (stateElapsedMs() >= 3000) {
            DisplayPrimary::showIdle(_machineName);
            enterState(State::IDLE);
        }
        break;

    // -----------------------------------------------------------------------
    case State::MACHINE_DISABLED:
        // Check if machine was re-enabled
        if (NVS::getMachineActive()) {
            MqttClient::publishStatus("idle", nullptr, 0, WiFi.RSSI());
            DisplayPrimary::showIdle(_machineName);
            enterState(State::IDLE);
        }
        break;

    // -----------------------------------------------------------------------
    case State::OTA_UPDATE:
        // OTA is handled at the top of tick() — shouldn't reach here normally
        break;

    // -----------------------------------------------------------------------
    case State::ERROR_STATE:
        // Watchdog will reset the device
        break;

    default:
        break;
    }

    updateStatusStrip();
}

State StateMachine::current() {
    return _state;
}
