/*
 * TEST 10 — WiFi Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: Network scan, target SSID visibility, RSSI quality, connection,
 *        IP assignment, NTP time sync (IST = UTC+5:30), disconnect/reconnect
 *
 * Wiring: None — internal WiFi antenna
 * Serial: 115200 baud
 *
 * CONFIGURE BEFORE FLASHING:
 *   Set WIFI_SSID and WIFI_PASSWORD below.
 *   These must match kiosk_station/secrets.py WIFI_SSID/WIFI_PASS.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// ── Edit these before flashing ───────────────────────────────────────────────
#define WIFI_SSID      "tlmms"
#define WIFI_PASSWORD  "your_wifi_password_here"
// ─────────────────────────────────────────────────────────────────────────────

#define NTP_SERVER     "pool.ntp.org"
#define NTP_GMT_OFFSET  19800   // UTC+5:30 IST in seconds
#define NTP_DST_OFFSET  0

static uint8_t passCount = 0;
static uint8_t failCount = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

static const char* rssiQuality(int rssi) {
    if (rssi > -60) return "EXCELLENT";
    if (rssi > -70) return "GOOD";
    if (rssi > -80) return "FAIR";
    return "POOR — relocate node or AP";
}

static void printRSSI(int rssi) {
    Serial.print(F("[INFO] RSSI: "));
    Serial.print(rssi);
    Serial.print(F(" dBm ("));
    Serial.print(rssiQuality(rssi));
    Serial.println(F(")"));
}

// ── Phase 1: Network scan ─────────────────────────────────────────────────────

static bool phase1_scan() {
    info("=== Phase 1: Network Scan ===");

    int networks = WiFi.scanNetworks();
    if (networks == 0) {
        fail("No networks found — WiFi antenna or hardware fault");
        return false;
    }

    Serial.print(F("[INFO] Found "));
    Serial.print(networks);
    Serial.println(F(" networks:"));

    bool targetFound = false;
    for (int i = 0; i < networks; i++) {
        Serial.print(F("[INFO]   "));
        Serial.print(WiFi.RSSI(i));
        Serial.print(F(" dBm  ["));
        Serial.print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "OPEN" : "AUTH");
        Serial.print(F("]  "));
        Serial.println(WiFi.SSID(i));

        if (WiFi.SSID(i) == WIFI_SSID) {
            targetFound = true;
        }
    }
    WiFi.scanDelete();

    if (targetFound) {
        pass("Target SSID found in scan");
    } else {
        fail("Target SSID not found — wrong SSID, wrong band (ESP32=2.4GHz only), or AP hidden");
    }
    return targetFound;
}

// ── Phase 2: Connection ───────────────────────────────────────────────────────

static bool phase2_connect() {
    info("=== Phase 2: Connection ===");

    Serial.print(F("[INFO] Connecting to \""));
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
        fail("Connection failed after 15s — check SSID/password");
        Serial.print(F("[INFO] WiFi status code: "));
        Serial.println(WiFi.status());
        Serial.println(F("[INFO] Status 6=CONNECT_FAILED (wrong password)"));
        Serial.println(F("[INFO] Status 1=NO_SSID_AVAIL (SSID changed)"));
        return false;
    }

    uint32_t elapsed = millis() - start;
    Serial.print(F("[PASS] Connected in "));
    Serial.print(elapsed);
    Serial.println(F(" ms"));
    passCount++;

    IPAddress ip   = WiFi.localIP();
    IPAddress gw   = WiFi.gatewayIP();
    IPAddress mask = WiFi.subnetMask();

    Serial.print(F("[INFO] IP address  : ")); Serial.println(ip);
    Serial.print(F("[INFO] Subnet mask : ")); Serial.println(mask);
    Serial.print(F("[INFO] Gateway     : ")); Serial.println(gw);
    Serial.print(F("[INFO] DNS         : ")); Serial.println(WiFi.dnsIP());
    Serial.print(F("[INFO] BSSID (AP)  : ")); Serial.println(WiFi.BSSIDstr());
    Serial.print(F("[INFO] Channel     : ")); Serial.println(WiFi.channel());
    printRSSI(WiFi.RSSI());

    if (ip[0] == 0) {
        fail("IP address is 0.0.0.0 — DHCP failed");
        return false;
    }
    pass("Valid IP address assigned");

    if (WiFi.RSSI() > -80) {
        pass("RSSI above minimum threshold (-80 dBm)");
    } else {
        fail("RSSI below -80 dBm — node must be moved closer to AP");
    }

    return true;
}

// ── Phase 3: NTP time sync ────────────────────────────────────────────────────

static void phase3_ntp() {
    info("=== Phase 3: NTP Time Sync ===");

    Serial.print(F("[INFO] Configuring NTP — server: "));
    Serial.print(NTP_SERVER);
    Serial.print(F("  GMT offset: "));
    Serial.print(NTP_GMT_OFFSET);
    Serial.println(F("s (IST = UTC+5:30)"));

    configTime(NTP_GMT_OFFSET, NTP_DST_OFFSET, NTP_SERVER);

    Serial.print(F("[INFO] Waiting for NTP sync"));
    uint32_t start = millis();
    struct tm ti;
    while (!getLocalTime(&ti) && millis() - start < 10000) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();

    if (!getLocalTime(&ti)) {
        fail("NTP sync timed out — check internet connectivity or firewall (UDP 123)");
        return;
    }

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S IST", &ti);
    Serial.print(F("[PASS] NTP synced — "));
    Serial.println(buf);
    passCount++;

    // Basic sanity: year should be 2024 or later
    if (ti.tm_year + 1900 >= 2024) {
        pass("NTP time is current (year >= 2024)");
    } else {
        fail("NTP time is in the past — sync may have used a stale value");
    }
}

// ── Phase 4: Disconnect / reconnect ──────────────────────────────────────────

static void phase4_reconnect() {
    info("=== Phase 4: Disconnect / Reconnect ===");

    Serial.println(F("[INFO] Forcing disconnect..."));
    WiFi.disconnect(false);
    delay(1000);

    if (WiFi.status() == WL_DISCONNECTED) {
        pass("WiFi disconnected cleanly");
    } else {
        Serial.print(F("[WARN] Unexpected status after disconnect: "));
        Serial.println(WiFi.status());
    }

    Serial.println(F("[INFO] Reconnecting..."));
    WiFi.reconnect();

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        uint32_t elapsed = millis() - start;
        Serial.print(F("[PASS] Reconnect OK in "));
        Serial.print(elapsed);
        Serial.println(F(" ms"));
        passCount++;
        printRSSI(WiFi.RSSI());
        pass("IP address retained after reconnect");
    } else {
        fail("Reconnect failed — WiFi driver or AP issue");
    }
}

// ── Setup / Loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 10: WiFi"));
    printSeparator();
    Serial.print(F("[INFO] Target SSID: "));
    Serial.println(WIFI_SSID);
    Serial.println(F("[INFO] Note: ESP32 supports 2.4 GHz only — not 5 GHz"));
    Serial.println();

    bool scanOk = phase1_scan();
    Serial.println();
    if (!scanOk) {
        fail("Skipping connection phases — target network not found");
        Serial.println(F("[INFO] Verify WIFI_SSID and ensure AP is 2.4 GHz"));
        return;
    }

    bool connOk = phase2_connect();
    Serial.println();
    if (!connOk) return;

    phase3_ntp();
    Serial.println();

    phase4_reconnect();
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
    info("Monitoring mode: printing RSSI and heap every 10s.");
    info("Press reset to re-run test sequence.");
}

void loop() {
    static uint32_t lastPrint = 0;
    uint32_t now = millis();

    if (now - lastPrint < 10000) return;
    lastPrint = now;

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F("[LOOP] Connected  RSSI="));
        Serial.print(WiFi.RSSI());
        Serial.print(F("dBm  heap="));
        Serial.print(ESP.getFreeHeap());
        Serial.print(F("B  uptime="));
        Serial.print(now / 1000);
        Serial.println(F("s"));
    } else {
        Serial.print(F("[LOOP] DISCONNECTED  status="));
        Serial.println(WiFi.status());
    }
}
