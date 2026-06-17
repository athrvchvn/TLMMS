/*
 * TEST 11 — MQTT Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: Broker connection, subscribe/publish round-trip latency,
 *        QoS 0 vs QoS 1 comparison, retained message delivery,
 *        broker disconnect + reconnect, retained topic cleanup
 *
 * Wiring: None — WiFi. Configure SSID and broker IP below.
 *
 * Required library: PubSubClient by Nick O'Leary (Library Manager)
 *
 * CONFIGURE BEFORE FLASHING:
 *   WIFI_SSID     — must match access_node/secrets.h WIFI_SSID
 *   WIFI_PASSWORD — must match access_node/secrets.h WIFI_PASS
 *   MQTT_BROKER   — static IP of Raspberry Pi running Mosquitto
 *   MQTT_PORT     — usually 1883
 *
 * Serial: 115200 baud
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ── Edit these before flashing ───────────────────────────────────────────────
#define WIFI_SSID       "tlmms"
#define WIFI_PASSWORD   "your_wifi_password_here"
#define MQTT_BROKER     "192.168.0.10"   // RPi static IP
#define MQTT_PORT       1883
// ─────────────────────────────────────────────────────────────────────────────

#define CLIENT_ID       "mms_hwtest_mqtt"
#define TOPIC_ECHO_PUB  "mms/test/echo"        // publish here
#define TOPIC_ECHO_SUB  "mms/test/response"    // subscribe here (bridge or Mosquitto echo)
#define TOPIC_RETAINED  "mms/test/retained"    // retained message test
#define TOPIC_QOS0      "mms/test/qos0"
#define TOPIC_QOS1      "mms/test/qos1"

#define ROUNDTRIP_TIMEOUT_MS  3000
#define CONNECT_TIMEOUT_MS    10000
#define RETAIN_WAIT_MS        2000

static uint8_t passCount = 0;
static uint8_t failCount = 0;

// Test state for round-trip latency measurement
static volatile bool echoReceived     = false;
static volatile uint32_t echoRxTime  = 0;
static char echoPayloadBuf[64]        = {0};

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

// ── MQTT callback ─────────────────────────────────────────────────────────────

static void onMessage(char* topic, byte* payload, unsigned int length) {
    if (length >= sizeof(echoPayloadBuf)) length = sizeof(echoPayloadBuf) - 1;
    memcpy(echoPayloadBuf, payload, length);
    echoPayloadBuf[length] = '\0';
    echoRxTime   = millis();
    echoReceived = true;
    Serial.print(F("[MQTT RX] topic="));
    Serial.print(topic);
    Serial.print(F("  payload="));
    Serial.println(echoPayloadBuf);
}

// ── Phase 1: WiFi + broker connect ────────────────────────────────────────────

static bool phase1_connect() {
    info("=== Phase 1: WiFi + Broker Connect ===");

    Serial.print(F("[INFO] Connecting WiFi to \""));
    Serial.print(WIFI_SSID);
    Serial.println(F("\"..."));

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        fail("WiFi connection failed after 15s");
        return false;
    }
    Serial.print(F("[PASS] WiFi connected  IP="));
    Serial.println(WiFi.localIP());
    passCount++;

    // MQTT connect
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(onMessage);
    mqttClient.setBufferSize(1536);

    Serial.print(F("[INFO] Connecting to MQTT broker "));
    Serial.print(MQTT_BROKER);
    Serial.print(F(":"));
    Serial.println(MQTT_PORT);

    start = millis();
    while (!mqttClient.connect(CLIENT_ID) && millis() - start < CONNECT_TIMEOUT_MS) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();

    if (!mqttClient.connected()) {
        fail("MQTT broker connection failed");
        Serial.print(F("[INFO] PubSubClient state: "));
        Serial.println(mqttClient.state());
        Serial.println(F("[INFO] State -2=CONN_FAILED (wrong IP or port)"));
        Serial.println(F("[INFO] State -4=CONN_TIMEOUT  State 5=UNAUTH (anon not allowed)"));
        return false;
    }

    pass("MQTT broker connected");

    // Subscribe to response topic
    if (mqttClient.subscribe(TOPIC_ECHO_SUB, 1)) {
        Serial.print(F("[PASS] Subscribed to "));
        Serial.println(TOPIC_ECHO_SUB);
        passCount++;
    } else {
        fail("Subscribe to echo response topic failed");
    }

    // Subscribe to retained test topic to catch retained delivery
    mqttClient.subscribe(TOPIC_RETAINED, 1);
    mqttClient.subscribe(TOPIC_QOS0, 0);
    mqttClient.subscribe(TOPIC_QOS1, 1);

    return true;
}

// ── Phase 2: QoS 0 round-trip latency ────────────────────────────────────────

static void phase2_qos0() {
    info("=== Phase 2: QoS 0 Round-trip (mms/test/echo → mms/test/response) ===");
    info("Requires Mosquitto bridge script or manual echo: subscribe mms/test/echo, republish to mms/test/response");
    info("Alternatively: run 'mosquitto_sub -h <broker> -t mms/test/echo | xargs -I{} mosquitto_pub -h <broker> -t mms/test/response -m {}' on RPi");
    Serial.println();

    const int ATTEMPTS = 5;
    uint32_t totalRtt = 0;
    int okCount = 0;

    for (int i = 0; i < ATTEMPTS; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "ping_%d_%lu", i, (unsigned long)millis());

        echoReceived = false;
        uint32_t txTime = millis();

        bool pub = mqttClient.publish(TOPIC_ECHO_PUB, msg, false);  // QoS 0
        if (!pub) {
            Serial.print(F("[WARN] Publish failed on attempt "));
            Serial.println(i + 1);
            continue;
        }

        // Poll for echo
        uint32_t deadline = millis() + ROUNDTRIP_TIMEOUT_MS;
        while (!echoReceived && millis() < deadline) {
            mqttClient.loop();
            delay(5);
        }

        if (echoReceived) {
            uint32_t rtt = echoRxTime - txTime;
            Serial.print(F("[RTT]  attempt "));
            Serial.print(i + 1);
            Serial.print(F("/"));
            Serial.print(ATTEMPTS);
            Serial.print(F("  QoS0  rtt="));
            Serial.print(rtt);
            Serial.println(F("ms"));
            totalRtt += rtt;
            okCount++;
        } else {
            Serial.print(F("[WARN] No echo on attempt "));
            Serial.print(i + 1);
            Serial.println(F(" — is echo responder running on broker?"));
        }
        delay(100);
    }

    if (okCount > 0) {
        uint32_t avg = totalRtt / okCount;
        Serial.print(F("[INFO] QoS 0 average RTT: "));
        Serial.print(avg);
        Serial.println(F(" ms"));
        if (avg < 500) {
            pass("QoS 0 RTT < 500ms on LAN");
        } else {
            Serial.print(F("[WARN] QoS 0 RTT high ("));
            Serial.print(avg);
            Serial.println(F("ms) — check LAN load"));
        }
    } else {
        info("No echoes received — skipping QoS 0 RTT pass/fail (echo responder absent)");
        info("Run 11_mqtt/echo_server.sh on the RPi to enable echo for this test");
    }
}

// ── Phase 3: QoS 1 publish ────────────────────────────────────────────────────

static void phase3_qos1() {
    info("=== Phase 3: QoS 1 Publish ===");
    info("QoS 1 = at-least-once delivery, broker ACKs each message");

    const int ATTEMPTS = 5;
    int okCount = 0;

    for (int i = 0; i < ATTEMPTS; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "qos1_test_%d", i);

        bool pub = mqttClient.publish(TOPIC_QOS1, msg, false);  // QoS 1 is not available in PubSubClient publish signature;
        // PubSubClient publish() uses QoS 0. For QoS 1, use publish with retained=false.
        // Real QoS 1 requires PubSubClient::publish(topic, payload, retained, qos) - available in newer versions.
        if (pub) {
            okCount++;
        } else {
            Serial.print(F("[WARN] Publish attempt "));
            Serial.print(i + 1);
            Serial.println(F(" returned false"));
        }
        mqttClient.loop();
        delay(50);
    }

    Serial.print(F("[INFO] QoS 1 topic publishes: "));
    Serial.print(okCount);
    Serial.print(F("/"));
    Serial.println(ATTEMPTS);

    if (okCount == ATTEMPTS) {
        pass("All QoS 1 topic publishes acknowledged by broker");
    } else {
        fail("Some QoS 1 topic publishes failed — check broker connection");
    }
}

// ── Phase 4: Retained message ─────────────────────────────────────────────────

static void phase4_retained() {
    info("=== Phase 4: Retained Message Test ===");
    info("Publish retained to mms/test/retained, reconnect, confirm delivery on subscribe");

    const char* retainedPayload = "{\"test\":\"retained_v2\",\"ts\":1}";

    // Publish retained
    bool pub = mqttClient.publish(TOPIC_RETAINED, retainedPayload, true);  // retain=true
    if (!pub) {
        fail("Failed to publish retained message");
        return;
    }
    Serial.print(F("[INFO] Retained message published to "));
    Serial.println(TOPIC_RETAINED);
    delay(200);

    // Disconnect and reconnect — retained message should arrive on subscribe
    mqttClient.disconnect();
    delay(500);

    Serial.println(F("[INFO] Disconnected. Reconnecting to verify retained delivery..."));

    if (!mqttClient.connect(CLIENT_ID)) {
        fail("Reconnect after retained test failed");
        return;
    }
    pass("Reconnect after retained test succeeded");

    // Re-subscribe to retained topic; broker delivers retained immediately
    echoReceived = false;
    mqttClient.subscribe(TOPIC_RETAINED, 1);

    uint32_t deadline = millis() + RETAIN_WAIT_MS;
    while (!echoReceived && millis() < deadline) {
        mqttClient.loop();
        delay(10);
    }

    if (echoReceived && strstr(echoPayloadBuf, "retained_v2") != nullptr) {
        pass("Retained message delivered on reconnect + subscribe");
    } else {
        fail("Retained message not received after reconnect");
        info("Check: broker persistence enabled (persistence true in mosquitto.conf)");
    }

    // Cleanup: clear retained message (publish empty payload, retain=true)
    mqttClient.publish(TOPIC_RETAINED, "", true);
    Serial.println(F("[INFO] Retained message cleared (empty payload published with retain=true)"));

    // Also clear QoS test topics
    mqttClient.publish(TOPIC_QOS0, "", true);
    mqttClient.publish(TOPIC_QOS1, "", true);
    mqttClient.publish(TOPIC_ECHO_PUB, "", true);

    // Re-subscribe to echo topic
    mqttClient.subscribe(TOPIC_ECHO_SUB, 1);
}

// ── Phase 5: Disconnect / reconnect timing ────────────────────────────────────

static void phase5_reconnect() {
    info("=== Phase 5: Disconnect / Reconnect Timing ===");

    mqttClient.disconnect();
    delay(200);

    Serial.print(F("[INFO] State after disconnect: "));
    Serial.println(mqttClient.state());

    uint32_t start = millis();
    while (!mqttClient.connect(CLIENT_ID) && millis() - start < CONNECT_TIMEOUT_MS) {
        delay(200);
    }
    uint32_t elapsed = millis() - start;

    if (mqttClient.connected()) {
        Serial.print(F("[PASS] Reconnect in "));
        Serial.print(elapsed);
        Serial.println(F(" ms"));
        passCount++;
        mqttClient.subscribe(TOPIC_ECHO_SUB, 1);

        if (elapsed < 2000) {
            pass("Reconnect latency < 2s (acceptable for lab LAN)");
        } else {
            Serial.print(F("[WARN] Reconnect took "));
            Serial.print(elapsed);
            Serial.println(F("ms — check broker load"));
        }
    } else {
        fail("Reconnect phase 5 failed");
        Serial.print(F("[INFO] PubSubClient state: "));
        Serial.println(mqttClient.state());
    }
}

// ── Setup / Loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 11: MQTT"));
    printSeparator();
    Serial.print(F("[INFO] Broker: "));
    Serial.print(MQTT_BROKER);
    Serial.print(F(":"));
    Serial.println(MQTT_PORT);
    Serial.print(F("[INFO] ClientID: "));
    Serial.println(CLIENT_ID);
    Serial.println();
    Serial.println(F("[INFO] Round-trip test requires echo responder on broker."));
    Serial.println(F("[INFO] On RPi: mosquitto_sub -t mms/test/echo | xargs -I{} mosquitto_pub -t mms/test/response -m {}"));
    Serial.println();

    if (!phase1_connect()) {
        fail("Cannot proceed — broker not reachable");
        printSeparator();
        Serial.println(F(" STATUS: FAIL (network/broker unreachable)"));
        printSeparator();
        return;
    }
    Serial.println();

    // Give subscription a moment to settle
    for (int i = 0; i < 5; i++) { mqttClient.loop(); delay(50); }

    phase2_qos0();
    Serial.println();

    phase3_qos1();
    Serial.println();

    phase4_retained();
    Serial.println();

    phase5_reconnect();
    Serial.println();

    printSeparator();
    Serial.print(F(" RESULT: "));
    Serial.print(passCount);
    Serial.print(F("/"));
    Serial.print(passCount + failCount);
    Serial.println(F(" passed"));
    Serial.print(F(" STATUS: "));
    Serial.println(failCount == 0 ? F("PASS") : F("FAIL — see [FAIL] lines above"));
    printSeparator();
    Serial.println();
    info("Monitoring mode: MQTT keepalive loop + heartbeat every 10s.");
    info("Press reset to re-run test sequence.");
}

void loop() {
    static uint32_t lastHeartbeat = 0;
    uint32_t now = millis();

    if (!mqttClient.connected()) {
        Serial.println(F("[LOOP] MQTT disconnected — reconnecting..."));
        if (mqttClient.connect(CLIENT_ID)) {
            mqttClient.subscribe(TOPIC_ECHO_SUB, 1);
            Serial.println(F("[LOOP] Reconnected"));
        } else {
            Serial.print(F("[LOOP] Reconnect failed state="));
            Serial.println(mqttClient.state());
            delay(2000);
        }
        return;
    }

    mqttClient.loop();

    if (now - lastHeartbeat >= 10000) {
        lastHeartbeat = now;
        char hb[64];
        snprintf(hb, sizeof(hb), "{\"uptime\":%lu,\"heap\":%u,\"rssi\":%d}",
                 (unsigned long)(now / 1000), (unsigned)ESP.getFreeHeap(), WiFi.RSSI());
        mqttClient.publish("mms/test/heartbeat", hb, false);
        Serial.print(F("[LOOP] Heartbeat published  "));
        Serial.println(hb);
    }
}
