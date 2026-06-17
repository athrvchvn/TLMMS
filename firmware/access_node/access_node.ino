/**
 * MMS V2 — Access Node Firmware
 * Tinkerers' Lab, IIT Indore
 *
 * Hardware: ESP32 NodeMCU 38-pin
 * Modules:  MFRC522, HC89 slot sensor, 128x64 SPI OLED,
 *           128x32 I2C OLED, Relay/SSR, WS2812 (Phase 3)
 *
 * Edit config.h and secrets.h before flashing each node.
 * See secrets.h.template for the gitignored secrets file.
 */

#include "config.h"
#include "secrets.h"
#include "nvs_manager.h"
#include "rfid_handler.h"
#include "slot_sensor.h"
#include "relay_controller.h"
#include "session_manager.h"
#include "mqtt_client.h"
#include "littlefs_logger.h"
#include "revocation_cache.h"
#include "display_primary.h"
#include "display_status.h"
#include "watchdog.h"
#include "state_machine.h"

#include <WiFi.h>
#include <time.h>
#include <esp_ota_ops.h>

// ---------------------------------------------------------------------------
// WiFi + NTP
// ---------------------------------------------------------------------------
static void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t deadline = millis() + 10000;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED) {
        configTime(19800, 0, "pool.ntp.org", "time.google.com");  // IST = UTC+5:30
        // Wait up to 5s for NTP sync
        struct tm ti;
        uint32_t ntpDeadline = millis() + 5000;
        while (!getLocalTime(&ti, 100) && millis() < ntpDeadline) {}
    }

    pinMode(PIN_WIFI_LED, OUTPUT);
    digitalWrite(PIN_WIFI_LED, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
}

// ---------------------------------------------------------------------------
// Mark OTA partition valid (rollback guard)
// ---------------------------------------------------------------------------
static void markOtaValid() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Hardware init order matters — relay first (safe state)
    NVS::begin();
    Relay::begin(NVS::getRelayActiveHigh());

    // Displays
    DisplayPrimary::begin();
    DisplayStatus::begin();

    // Peripherals
    RFIDHandler::begin();
    SlotSensor::begin();
    Logger::begin();
    Revocation::begin();
    Session::begin();

    // Network
    connectWiFi();

    // MQTT
    MqttClient::begin(NVS::getMachineId());

    // Publish boot event (triggers orphaned session cleanup in bridge.py)
    MqttClient::publishBootEvent(NVS::getMachineId());
    Logger::appendBoot(NVS::getMachineId());

    // Watchdog — start last so all init is covered
    Watchdog::begin();

    // Mark OTA partition valid (prevents rollback after successful boot)
    markOtaValid();

    // State machine
    StateMachine::begin();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
    Watchdog::feed();
    StateMachine::tick();

    // Maintain WiFi LED
    digitalWrite(PIN_WIFI_LED, WiFi.status() == WL_CONNECTED ? HIGH : LOW);

    // Periodic NTP resync
    static uint32_t lastNtpMs = 0;
    if (WiFi.status() == WL_CONNECTED && millis() - lastNtpMs > NTP_SYNC_INTERVAL_MS) {
        lastNtpMs = millis();
        configTime(19800, 0, "pool.ntp.org");
    }
}
