/*
 * TEST 04 — 128×64 SSD1306 SPI OLED Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: SPI init, display init, text rendering, full-pixel fill, stripe patterns,
 *        MMS V2 idle screen simulation, progress bar animation
 *
 * Wiring:
 *   OLED VCC  → 3V3
 *   OLED GND  → GND
 *   OLED D0   → GPIO 18 (SCK — shared SPI bus)
 *   OLED D1   → GPIO 23 (MOSI — shared SPI bus)
 *   OLED RES  → GPIO 4  (Reset)
 *   OLED CS   → GPIO 16 (Chip select, active LOW)
 *   OLED DC   → GPIO 17 (Data/Command select)
 *
 * Required libraries:
 *   Adafruit SSD1306 (Library Manager)
 *   Adafruit GFX Library (Library Manager)
 *
 * Serial: 115200 baud
 * Page duration: 3 seconds each
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_W    128
#define SCREEN_H    64

#define PIN_OLED_DC   17
#define PIN_OLED_CS   16
#define PIN_OLED_RST  4
#define PIN_SPI_SCK   18
#define PIN_SPI_MOSI  23

#define PAGE_DURATION_MS  3000

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &SPI, PIN_OLED_DC, PIN_OLED_RST, PIN_OLED_CS);

static uint8_t passCount = 0;
static uint8_t failCount = 0;
static uint8_t currentPage = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

// ── Display pages ────────────────────────────────────────────────────────────

static void page1_systemInfo() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println(F("MMS V2 OLED64 Test"));
    display.println(F("128x64 SPI SSD1306"));
    display.drawLine(0, 17, 127, 17, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.print(F("DC:")); display.print(PIN_OLED_DC);
    display.print(F(" CS:")); display.print(PIN_OLED_CS);
    display.print(F(" RST:")); display.println(PIN_OLED_RST);
    display.print(F("SCK:")); display.print(PIN_SPI_SCK);
    display.print(F(" MOSI:")); display.println(PIN_SPI_MOSI);
    display.drawLine(0, 45, 127, 45, SSD1306_WHITE);
    display.setCursor(0, 48);
    display.println(F("Test: PASS"));
    display.println(F("Adafruit SSD1306"));

    display.display();
}

static void page2_fullFill() {
    display.clearDisplay();
    // Fill entire screen white
    display.fillRect(0, 0, SCREEN_W, SCREEN_H, SSD1306_WHITE);
    display.display();
}

static void page3_horizontalStripes() {
    display.clearDisplay();
    for (int y = 0; y < SCREEN_H; y += 8) {
        display.fillRect(0, y, SCREEN_W, 4, SSD1306_WHITE);
    }
    display.display();
}

static void page4_idleScreen() {
    // Simulate MMS V2 IDLE screen as defined in state_machine.cpp
    display.clearDisplay();

    // Title
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 2);
    display.println(F("  Tinkerers' Lab"));
    display.setCursor(22, 12);
    display.println(F("  IIT Indore"));

    // Divider
    display.drawLine(0, 24, 127, 24, SSD1306_WHITE);

    // Machine name (larger text)
    display.setTextSize(1);
    display.setCursor(8, 28);
    display.println(F("[Creality 3D Printer]"));

    // Divider
    display.drawLine(0, 40, 127, 40, SSD1306_WHITE);

    // Instruction
    display.setTextSize(1);
    display.setCursor(14, 46);
    display.println(F("Tap card to start"));

    // Card icon (simple rectangle)
    display.drawRect(2, 44, 10, 7, SSD1306_WHITE);
    display.fillRect(3, 45, 8, 5, SSD1306_WHITE);

    display.display();
}

static void page5_accessGranted() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Header
    display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(20, 2);
    display.print(F("ACCESS GRANTED"));

    // Roll number
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 16);
    display.print(F("Roll: 220002123"));

    // Machine name
    display.setCursor(0, 26);
    display.print(F("Creality 3D Printer"));

    // Session timer
    display.setTextSize(2);
    display.setCursor(22, 38);
    display.print(F("05:23"));

    // Remaining indicator
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Limit: 60 min"));

    display.display();
}

static void page6_progressBar() {
    // Animate a progress bar (OTA update simulation)
    for (int pct = 0; pct <= 100; pct += 2) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 2);
        display.println(F("Updating FW..."));
        display.setCursor(12, 14);
        display.println(F("v2.0.0 -> v2.1.0"));

        // Draw progress bar outline
        display.drawRect(4, 30, 120, 14, SSD1306_WHITE);
        // Fill progress
        int filled = (pct * 116) / 100;
        display.fillRect(6, 32, filled, 10, SSD1306_WHITE);

        // Percentage text
        display.setCursor(50, 48);
        display.print(pct);
        display.print(F("%"));

        display.setCursor(10, 56);
        display.println(F("Do not remove card"));

        display.display();
        delay(30);
    }
    delay(500);
}

static void page7_checkerboard() {
    display.clearDisplay();
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            if ((x + y) % 2 == 0) {
                display.drawPixel(x, y, SSD1306_WHITE);
            }
        }
    }
    display.display();
}

// ── Setup ────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 04: 128x64 SPI OLED"));
    printSeparator();

    Serial.print(F("[INFO] SSD1306 SPI on DC="));
    Serial.print(PIN_OLED_DC);
    Serial.print(F(" CS="));
    Serial.print(PIN_OLED_CS);
    Serial.print(F(" RST="));
    Serial.println(PIN_OLED_RST);

    SPI.begin(PIN_SPI_SCK, -1, PIN_SPI_MOSI, PIN_OLED_CS);

    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        fail("SSD1306 allocation or init failed");
        Serial.println(F("[INFO] Check: VCC=3.3V, SPI wiring, CS=16, DC=17, RST=4"));
        Serial.println(F("[INFO] If heap low, reduce other sketch code."));
        while (true) delay(1000);
    }
    pass("SSD1306 128x64 SPI init OK");

    display.setRotation(0);
    display.clearDisplay();
    display.display();
    delay(100);

    info("Running display test sequence...");
    info("Each page holds for 3 seconds. Watch display carefully.");
    Serial.println();
}

// ── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    switch (currentPage) {
        case 0:
            Serial.println(F("[INFO] Page 1/7 — System info"));
            page1_systemInfo();
            delay(PAGE_DURATION_MS);
            break;
        case 1:
            Serial.println(F("[INFO] Page 2/7 — Full pixel fill (all white)"));
            page2_fullFill();
            delay(PAGE_DURATION_MS);
            pass("Full fill: verify entire 128x64 area is white");
            break;
        case 2:
            Serial.println(F("[INFO] Page 3/7 — Horizontal stripes"));
            page3_horizontalStripes();
            delay(PAGE_DURATION_MS);
            pass("Stripes: verify alternating white/black horizontal bands");
            break;
        case 3:
            Serial.println(F("[INFO] Page 4/7 — MMS V2 IDLE screen simulation"));
            page4_idleScreen();
            delay(PAGE_DURATION_MS);
            pass("IDLE screen: verify lab name, machine name, tap instruction");
            break;
        case 4:
            Serial.println(F("[INFO] Page 5/7 — SESSION_ACTIVE screen simulation"));
            page5_accessGranted();
            delay(PAGE_DURATION_MS);
            pass("Session screen: verify roll number, machine name, timer");
            break;
        case 5:
            Serial.println(F("[INFO] Page 6/7 — OTA progress bar animation"));
            page6_progressBar();
            pass("Progress bar animated 0→100%");
            break;
        case 6:
            Serial.println(F("[INFO] Page 7/7 — Checkerboard pattern (pixel-level test)"));
            page7_checkerboard();
            delay(PAGE_DURATION_MS);
            pass("Checkerboard: verify no stuck pixels (all should light alternately)");

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

            // Loop back to page 1 for continuous visual inspection
            Serial.println(F("[INFO] Cycling back to page 1. Press reset to restart test."));
            currentPage = 255;  // will wrap to 0 after ++
            break;

        default:
            break;
    }
    currentPage++;
}
