/*
 * TEST 08 — WS2812 RGB LED Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: Individual RGB channels, white, off, color wheel, brightness sweep,
 *        MMS V2 state color simulation (idle breathe, active, warning, denied)
 *
 * Wiring:
 *   WS2812 VCC  → 5V (ESP32 Vin — USB-sourced only)
 *   WS2812 GND  → GND
 *   WS2812 DIN  → GPIO 33 via 470Ω series resistor
 *   [100µF electrolytic capacitor across VCC–GND at LED module, + toward VCC]
 *
 * IMPORTANT NOTES:
 *   - 5V required for LED power; 3.3V will be too dim and may cause color errors
 *   - 470Ω series resistor on data line protects against ringing
 *   - WS2812B works with 3.3V data; original WS2812 may need level shifter
 *   - LED_TYPE: default NEO_GRB — change to NEO_RGB if red shows as green
 *
 * Required library: Adafruit NeoPixel (Library Manager)
 * Serial: 115200 baud
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN_LED      33
#define NUM_LEDS     1

// Change NEO_GRB to NEO_RGB if colors appear wrong
#define LED_TYPE     (NEO_GRB + NEO_KHZ800)

Adafruit_NeoPixel strip(NUM_LEDS, PIN_LED, LED_TYPE);

static uint8_t passCount = 0;
static uint8_t failCount = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }

static void setColor(uint8_t r, uint8_t g, uint8_t b) {
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

// Color wheel: position 0–255 → RGB
static uint32_t wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85)  return strip.Color(255 - pos * 3, 0,           pos * 3);
    if (pos < 170) { pos -= 85;  return strip.Color(0, pos * 3,   255 - pos * 3); }
    pos -= 170;    return strip.Color(pos * 3,  255 - pos * 3, 0);
}

// Sine-wave brightness for breathing effect (0–127 range)
static uint8_t breathe(uint32_t ms, uint32_t period) {
    float t   = (float)(ms % period) / period;
    float val = (sin(t * 2 * PI - PI / 2) + 1.0f) / 2.0f;
    return (uint8_t)(val * 127);
}

// ── Test sections ────────────────────────────────────────────────────────────

static void testBasicColors() {
    Serial.println(F("[INFO] === Basic Color Test (2s per color) ==="));

    struct { uint8_t r, g, b; const char* name; } colors[] = {
        {255,   0,   0, "RED   (255,   0,   0)"},
        {  0, 255,   0, "GREEN (  0, 255,   0)"},
        {  0,   0, 255, "BLUE  (  0,   0, 255)"},
        {255, 255, 255, "WHITE (255, 255, 255)"},
        {255, 165,   0, "ORANGE(255, 165,   0)"},
        {255, 255,   0, "YELLOW(255, 255,   0)"},
        {  0,   0,   0, "OFF   (  0,   0,   0)"},
    };

    for (auto& c : colors) {
        Serial.print(F("[INFO] "));
        Serial.println(c.name);
        setColor(c.r, c.g, c.b);
        delay(2000);
    }
    pass("Basic color sequence complete — confirm each color visually");
}

static void testColorWheel() {
    Serial.println(F("[INFO] === Color Wheel Sweep (256 steps) ==="));
    for (int i = 0; i < 256; i++) {
        strip.setPixelColor(0, wheel((uint8_t)i));
        strip.show();
        delay(10);
    }
    setColor(0, 0, 0);
    pass("Color wheel sweep complete — confirm full hue spectrum with no gaps");
}

static void testBrightnessSweep() {
    Serial.println(F("[INFO] === Brightness Sweep (Red channel, 0→255→0) ==="));
    // Ramp up
    for (int b = 0; b <= 255; b++) {
        strip.setPixelColor(0, strip.Color(b, 0, 0));
        strip.show();
        delay(5);
    }
    // Ramp down
    for (int b = 255; b >= 0; b--) {
        strip.setPixelColor(0, strip.Color(b, 0, 0));
        strip.show();
        delay(5);
    }
    pass("Brightness sweep — confirm smooth fade, no abrupt jumps");
}

static void testMmsStates() {
    Serial.println(F("[INFO] === MMS V2 State Color Simulation ==="));
    uint32_t start = millis();

    // IDLE: dim white breathing (4s)
    Serial.println(F("[INFO] IDLE state — dim white breathing (4s)"));
    start = millis();
    while (millis() - start < 4000) {
        uint8_t b = breathe(millis(), 2000);
        strip.setPixelColor(0, strip.Color(b, b, b));
        strip.show();
        delay(16);
    }
    pass("IDLE: white breathing visible");

    // SESSION_ACTIVE: solid green (2s)
    Serial.println(F("[INFO] SESSION_ACTIVE — solid green (2s)"));
    setColor(0, 255, 0);
    delay(2000);
    pass("SESSION_ACTIVE: solid green");

    // TIMEOUT_WARNING: alternate yellow ↔ orange (4s)
    Serial.println(F("[INFO] TIMEOUT_WARNING — alternating yellow/orange (4s)"));
    start = millis();
    while (millis() - start < 4000) {
        uint32_t t = millis() - start;
        if ((t / 500) % 2 == 0) setColor(255, 255, 0);   // yellow
        else                    setColor(255, 165, 0);   // orange
        delay(50);
    }
    pass("TIMEOUT_WARNING: yellow/orange alternation visible");

    // ACCESS_DENIED: 3× quick red flash
    Serial.println(F("[INFO] ACCESS_DENIED — 3x red flash"));
    for (int i = 0; i < 3; i++) {
        setColor(255, 0, 0);
        delay(150);
        setColor(0, 0, 0);
        delay(150);
    }
    pass("ACCESS_DENIED: 3 red flashes");

    // MACHINE_DISABLED: dim purple
    Serial.println(F("[INFO] MACHINE_DISABLED — dim purple (2s)"));
    setColor(80, 0, 80);
    delay(2000);
    pass("MACHINE_DISABLED: dim purple visible");

    // OTA_UPDATE: blue chasing animation (3s)
    Serial.println(F("[INFO] OTA_UPDATE — blue fade (3s)"));
    start = millis();
    while (millis() - start < 3000) {
        uint8_t b = breathe(millis(), 500);
        setColor(0, 0, b * 2);
        delay(16);
    }
    pass("OTA_UPDATE: blue pulse visible");

    // All off
    setColor(0, 0, 0);
}

// ── Setup / Loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 08: WS2812 RGB LED"));
    printSeparator();

    Serial.print(F("[INFO] GPIO "));
    Serial.print(PIN_LED);
    Serial.print(F(" → 470Ω → DIN   VCC=5V  NUM_LEDS="));
    Serial.println(NUM_LEDS);
    Serial.println(F("[INFO] LED type: NEO_GRB (change to NEO_RGB if colors wrong)"));
    Serial.println();
    Serial.println(F("[SAFETY] Check: 100µF cap across VCC-GND at LED module"));
    Serial.println(F("[SAFETY] Check: 5V on VCC pin (not 3.3V)"));
    Serial.println();

    strip.begin();
    strip.setBrightness(255);
    strip.clear();
    strip.show();
    delay(100);

    // Quick power-on sanity: white flash
    Serial.println(F("[INFO] Brief white flash — confirm LED is wired and powered"));
    setColor(128, 128, 128);
    delay(500);
    setColor(0, 0, 0);
    delay(200);

    Serial.println();
    testBasicColors();
    Serial.println();
    testColorWheel();
    Serial.println();
    testBrightnessSweep();
    Serial.println();
    testMmsStates();
    Serial.println();

    printSeparator();
    Serial.print(F(" RESULT: "));
    Serial.print(passCount);
    Serial.print(F("/"));
    Serial.print(passCount + failCount);
    Serial.println(F(" automated checks (all require visual confirmation)"));
    Serial.println(F(" STATUS: PASS if all colors and animations were visible correctly"));
    printSeparator();

    Serial.println();
    Serial.println(F("[INFO] Entering loop — cycling MMS V2 state animations indefinitely."));
    Serial.println(F("[INFO] Press reset to re-run full test sequence."));
}

void loop() {
    // Continuous breathing while idle — useful for long-term stability check
    uint8_t b = breathe(millis(), 2000);
    strip.setPixelColor(0, strip.Color(b, b, b));
    strip.show();
    delay(16);
}
