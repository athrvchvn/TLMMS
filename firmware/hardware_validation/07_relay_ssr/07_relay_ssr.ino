/*
 * TEST 07 — Solid State Relay (SSR) Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: 3.3V GPIO drive on SSR control input, ON/OFF switching,
 *        status LED observation, 10-cycle test, safe-state-on-boot
 *
 * Wiring (e.g. Fotek SSR-25 DA or equivalent):
 *   SSR INPUT+  → GPIO 26       (3.3V GPIO drives directly — no resistor needed)
 *   SSR INPUT-  → GND
 *   SSR LOAD1   → Multimeter probe A
 *   SSR LOAD2   → Multimeter probe B
 *
 * SAFETY: DO NOT connect mains voltage. Use multimeter only.
 *
 * Key differences from mechanical relay (Test 06):
 *   - No audible click — use multimeter and STATUS LED on SSR
 *   - 3.3V GPIO drives directly (no optocoupler module needed)
 *   - AC SSR switches at zero-crossing — fast ON, not instant OFF
 *   - Some SSRs have 1–3V off-state leakage — normal
 *
 * Serial: 115200 baud
 * Commands: same as Test 06 (h/l/o/f/p/c/s/?)
 */

#include <Arduino.h>

#define PIN_RELAY       26
#define CYCLE_COUNT     10
#define CYCLE_ON_MS     500     // Longer than mechanical — zero-cross switching
#define CYCLE_OFF_MS    500
#define PULSE_MS        1000

static bool activeHigh = true;
static bool ssrIsOn    = false;
static uint8_t passCount = 0;
static uint8_t failCount = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

static void setSSR(bool on) {
    ssrIsOn = on;
    if (activeHigh) {
        digitalWrite(PIN_RELAY, on ? HIGH : LOW);
    } else {
        digitalWrite(PIN_RELAY, on ? LOW : HIGH);
    }
}

static void printStatus() {
    Serial.print(F("[STATE] SSR "));
    Serial.print(ssrIsOn ? F("ON ") : F("OFF"));
    Serial.print(F("  GPIO26="));
    Serial.print(digitalRead(PIN_RELAY) == HIGH ? F("3.3V") : F("0V  "));
    Serial.print(F("  mode=active_"));
    Serial.println(activeHigh ? F("HIGH") : F("LOW "));
    Serial.println(F("[CHECK] SSR control LED: illuminated=ON, off=OFF"));
    Serial.println(F("[CHECK] Multimeter on LOAD terminals: <1Ω=ON, open=OFF"));
}

static void printHelp() {
    Serial.println(F("[INFO] Commands (115200 baud):"));
    Serial.println(F("[INFO]   h = active HIGH mode (GPIO HIGH = SSR ON) [default]"));
    Serial.println(F("[INFO]   l = active LOW mode  (GPIO LOW  = SSR ON)"));
    Serial.println(F("[INFO]   o = SSR ON"));
    Serial.println(F("[INFO]   f = SSR OFF"));
    Serial.println(F("[INFO]   p = 1s pulse"));
    Serial.println(F("[INFO]   c = 10-cycle test (500ms on/off)"));
    Serial.println(F("[INFO]   s = print status"));
    Serial.println(F("[INFO]   ? = this help"));
}

static void printSSRNotes() {
    Serial.println(F("── SSR vs Mechanical Relay ─────────────────────────────────────────"));
    Serial.println(F("[INFO] SSR differences:"));
    Serial.println(F("[INFO]   No click sound — use LED on SSR and multimeter to confirm"));
    Serial.println(F("[INFO]   3.3V GPIO drives INPUT+ directly — no extra transistor needed"));
    Serial.println(F("[INFO]   AC SSR: zero-cross switch → ON within half cycle (~10ms@50Hz)"));
    Serial.println(F("[INFO]   Off-state leakage: 1–3V on LOAD terminals is NORMAL for SSR"));
    Serial.println(F("[INFO]   If load terminals measure full supply voltage when 'off',"));
    Serial.println(F("[INFO]   SSR is welded — replace immediately"));
    Serial.println(F("──────────────────────────────────────────────────────────────────────"));
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);

    // Safe state: GPIO LOW before enabling output
    digitalWrite(PIN_RELAY, LOW);
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);
    delay(50);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 07: Solid State Relay (SSR)"));
    printSeparator();

    bool bootLow = (digitalRead(PIN_RELAY) == LOW);
    if (bootLow) {
        pass("GPIO 26 starts LOW — SSR off on boot (safe state)");
    } else {
        fail("GPIO 26 not LOW on boot");
    }

    Serial.println();
    Serial.println(F("[SAFETY] DO NOT connect mains voltage during this test."));
    Serial.println(F("[SAFETY] Use multimeter in OHM or CONTINUITY mode on LOAD terminals."));
    Serial.println();

    printSSRNotes();
    Serial.println();
    printHelp();
    Serial.println();

    // Measure 3.3V output capability
    Serial.println(F("[INFO] Measuring GPIO 26 output voltage:"));
    digitalWrite(PIN_RELAY, HIGH);
    delay(10);
    Serial.println(F("[INFO]   GPIO HIGH — measure voltage at SSR INPUT+"));
    Serial.println(F("[INFO]   Expected: 2.8–3.3V. SSR control spec: 3–32V DC."));
    Serial.println(F("[INFO]   Some SSRs need 5V minimum — check datasheet."));
    delay(500);
    digitalWrite(PIN_RELAY, LOW);
    Serial.println(F("[INFO]   GPIO LOW — SSR should deactivate."));
    Serial.println();

    printStatus();
    Serial.println();
    info("Send commands to test relay switching.");
}

// ── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    if (!Serial.available()) return;

    char cmd = (char)Serial.read();
    while (Serial.available()) Serial.read();

    switch (cmd) {
        case 'h':
            activeHigh = true;
            setSSR(false);
            info("Mode: ACTIVE HIGH — GPIO 3.3V = SSR ON");
            printStatus();
            break;

        case 'l':
            activeHigh = false;
            setSSR(false);
            info("Mode: ACTIVE LOW — GPIO 0V = SSR ON");
            printStatus();
            break;

        case 'o':
            setSSR(true);
            Serial.println(F("[ACTION] SSR ON — GPIO26 = 3.3V"));
            printStatus();
            break;

        case 'f':
            setSSR(false);
            Serial.println(F("[ACTION] SSR OFF — GPIO26 = 0V"));
            printStatus();
            Serial.println(F("[INFO] Note: up to 3V leakage on LOAD terminals is normal for SSR"));
            break;

        case 'p':
            Serial.println(F("[ACTION] Pulse: SSR ON for 1000ms then OFF"));
            setSSR(true);
            printStatus();
            delay(PULSE_MS);
            setSSR(false);
            printStatus();
            pass("Pulse complete");
            break;

        case 'c': {
            Serial.println(F("[INFO] 10-cycle test: 500ms ON / 500ms OFF"));
            Serial.println(F("[INFO] Watch SSR status LED — 10 ON/OFF transitions expected"));
            for (int i = 1; i <= CYCLE_COUNT; i++) {
                setSSR(true);
                Serial.print(F("[CYCLE] ")); Serial.print(i); Serial.print(F("/10 ON  GPIO="));
                Serial.println(digitalRead(PIN_RELAY) == HIGH ? F("HIGH") : F("LOW"));
                delay(CYCLE_ON_MS);

                setSSR(false);
                Serial.print(F("[CYCLE] ")); Serial.print(i); Serial.print(F("/10 OFF GPIO="));
                Serial.println(digitalRead(PIN_RELAY) == HIGH ? F("HIGH") : F("LOW"));
                delay(CYCLE_OFF_MS);
            }
            pass("10-cycle test complete");
            Serial.println(F("[CHECK] Confirm: SSR LED flashed 10 times"));
            Serial.println(F("[CHECK] Confirm: Multimeter showed low resistance ON, open OFF"));
            setSSR(false);
            break;
        }

        case 's':
            printStatus();
            break;

        case '?':
            printHelp();
            break;

        case '\r': case '\n': break;

        default:
            Serial.print(F("[WARN] Unknown: '")); Serial.print(cmd); Serial.println(F("' — send '?'"));
            break;
    }
}
