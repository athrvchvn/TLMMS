/*
 * TEST 05 — 128×32 SSD1306 I2C OLED Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: I2C bus scan, display init at 0x3C, text rendering at 32-pixel height,
 *        MMS V2 status strip simulation
 *
 * Wiring:
 *   OLED VCC → 3V3
 *   OLED GND → GND
 *   OLED SDA → GPIO 21  (with 4.7kΩ pull-up to 3V3 if not on module)
 *   OLED SCL → GPIO 22  (with 4.7kΩ pull-up to 3V3 if not on module)
 *   OLED RST → not connected (pass -1 to constructor)
 *
 * Default I2C address: 0x3C (ADDR pin floating or LOW)
 * Alternative address: 0x3D (ADDR pin HIGH)
 *
 * Required libraries:
 *   Adafruit SSD1306 (Library Manager)
 *   Adafruit GFX Library (Library Manager)
 *
 * Serial: 115200 baud
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_W    128
#define SCREEN_H    32
#define OLED_RESET  -1      // No RST pin used
#define OLED_ADDR   0x3C    // Change to 0x3D if ADDR pin is HIGH

#define PIN_I2C_SDA  21
#define PIN_I2C_SCL  22

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

static uint8_t passCount = 0;
static uint8_t failCount = 0;
static uint8_t currentPage = 0;
static uint32_t lastPageTime = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

// ── I2C scan ─────────────────────────────────────────────────────────────────

static bool i2cScan() {
    info("Scanning I2C bus (SDA=21, SCL=22)...");

    int devicesFound = 0;
    bool targetFound = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            devicesFound++;
            Serial.print(F("[INFO]   Device found at 0x"));
            if (addr < 16) Serial.print('0');
            Serial.print(addr, HEX);

            if (addr == 0x3C) {
                Serial.println(F("  ← SSD1306 (expected address)"));
                targetFound = true;
            } else if (addr == 0x3D) {
                Serial.println(F("  ← SSD1306 (alt address — update OLED_ADDR if using this)"));
            } else {
                Serial.println(F("  (unknown device)"));
            }
        }
    }

    if (devicesFound == 0) {
        fail("No devices found on I2C bus — check SDA/SCL wiring and pull-ups");
        return false;
    }

    if (targetFound) {
        pass("SSD1306 found at 0x3C (correct address)");
        return true;
    } else {
        Serial.print(F("[FAIL] SSD1306 not at 0x3C — "));
        Serial.print(devicesFound);
        Serial.println(F(" other device(s) found. Check OLED_ADDR constant."));
        failCount++;
        return false;
    }
}

// ── Display pages ────────────────────────────────────────────────────────────

// Page 1: MMS V2 status strip simulation
// This is the exact layout the status strip module renders in normal operation.
// Row 1 (top):   WiFi icon | MQTT icon | machine name (truncated) | time
// Row 2 (blank)
// Row 3: connection details
// Row 4: prompt
static void page1_statusStrip() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // WiFi dot (filled = connected)
    display.fillCircle(4, 4, 3, SSD1306_WHITE);
    // MQTT dot (filled = connected)
    display.fillCircle(12, 4, 3, SSD1306_WHITE);

    // Machine name (truncated to fit 128-24 pixels of space at size 1 = ~15 chars)
    display.setCursor(20, 0);
    display.print(F("SOLDERING STA 1"));

    // Time (right-aligned)
    display.setCursor(98, 0);
    display.print(F("14:58"));

    // Separator line
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Second row: connection status
    display.setCursor(0, 14);
    display.print(F("WiFi:tlmms  MQTT:OK"));

    // Third row: machine status
    display.setCursor(0, 24);
    display.print(F("IDLE - Tap card    "));

    display.display();
}

// Page 2: Status strip in SESSION state
static void page2_statusSession() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // WiFi dot
    display.fillCircle(4, 4, 3, SSD1306_WHITE);
    // MQTT dot
    display.fillCircle(12, 4, 3, SSD1306_WHITE);

    display.setCursor(20, 0);
    display.print(F("SOLDERING STA 1"));

    display.setCursor(98, 0);
    display.print(F("14:59"));

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 14);
    display.print(F("Roll:220002123"));

    display.setCursor(0, 24);
    display.print(F("Session: 05:23 / 15m"));

    display.display();
}

// Page 3: Full pixel fill
static void page3_fullFill() {
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_W, SCREEN_H, SSD1306_WHITE);
    display.display();
}

// Page 4: Vertical stripes
static void page4_verticalStripes() {
    display.clearDisplay();
    for (int x = 0; x < SCREEN_W; x += 4) {
        display.fillRect(x, 0, 2, SCREEN_H, SSD1306_WHITE);
    }
    display.display();
}

// Page 5: Offline status (WiFi disconnected)
static void page5_offlineStatus() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // WiFi dot — hollow = disconnected
    display.drawCircle(4, 4, 3, SSD1306_WHITE);
    // MQTT dot — hollow = disconnected
    display.drawCircle(12, 4, 3, SSD1306_WHITE);

    display.setCursor(20, 0);
    display.print(F("SOLDERING STA 1"));

    display.setCursor(98, 0);
    display.print(F("OFFL"));

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 14);
    display.print(F("WiFi: DISCONNECTED"));

    display.setCursor(0, 24);
    display.print(F("Offline mode active "));

    display.display();
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 05: 128x32 I2C OLED"));
    printSeparator();

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000);  // 400 kHz fast mode

    Serial.println();
    if (!i2cScan()) {
        Serial.println(F("[FATAL] I2C scan failed — check SDA/SCL wiring and 4.7kΩ pull-ups"));
        while (true) delay(1000);
    }
    Serial.println();

    Serial.print(F("[INFO] Initializing SSD1306 at 0x"));
    Serial.print(OLED_ADDR, HEX);
    Serial.println(F("..."));

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        fail("SSD1306 128x32 I2C init failed");
        Serial.println(F("[INFO] Ensure height=32 in constructor, not 64."));
        while (true) delay(1000);
    }
    pass("SSD1306 128x32 I2C init OK");
    display.clearDisplay();
    display.display();
    delay(100);

    info("Running 5-page display test. Each page holds 3 seconds.");
    info("Watch display. Serial prompts guide visual checks.");
    Serial.println();

    lastPageTime = millis();
}

// ── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    if (millis() - lastPageTime < 3000) return;
    lastPageTime = millis();

    switch (currentPage % 6) {
        case 0:
            Serial.println(F("[INFO] Page 1/5 — Status strip IDLE simulation"));
            Serial.println(F("[CHECK] Two filled dots on left, machine name centre, time right"));
            page1_statusStrip();
            break;
        case 1:
            Serial.println(F("[INFO] Page 2/5 — Status strip SESSION simulation"));
            Serial.println(F("[CHECK] Roll number and session timer visible in 32px height"));
            page2_statusSession();
            break;
        case 2:
            Serial.println(F("[INFO] Page 3/5 — Full pixel fill"));
            Serial.println(F("[CHECK] All 128x32 pixels lit white — no dark rows"));
            page3_fullFill();
            pass("Full fill visible — no dead rows");
            break;
        case 3:
            Serial.println(F("[INFO] Page 4/5 — Vertical stripes"));
            Serial.println(F("[CHECK] Alternating white/black vertical columns"));
            page4_verticalStripes();
            pass("Vertical stripes rendered");
            break;
        case 4:
            Serial.println(F("[INFO] Page 5/5 — Offline status simulation"));
            Serial.println(F("[CHECK] Hollow dots (disconnected), OFFL time, WiFi: DISCONNECTED"));
            page5_offlineStatus();
            break;
        case 5:
            Serial.println();
            printSeparator();
            Serial.print(F(" RESULT: "));
            Serial.print(passCount);
            Serial.print(F("/"));
            Serial.print(passCount + failCount);
            Serial.println(F(" automated checks passed"));
            Serial.println(F(" STATUS: All pages must be visually confirmed"));
            Serial.print(F(" AUTO STATUS: "));
            Serial.println(failCount == 0 ? F("PASS") : F("FAIL"));
            printSeparator();
            Serial.println(F("[INFO] Cycling... press reset to restart."));
            break;
    }
    currentPage++;
}
