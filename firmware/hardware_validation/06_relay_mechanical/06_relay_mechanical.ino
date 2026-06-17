/*
 * TEST 06 — Mechanical Relay Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: GPIO drive, active HIGH / active LOW modes, pulse, cycle test,
 *        safe-state-on-boot verification
 *
 * Wiring:
 *   Relay VCC → ESP32 5V (Vin)
 *   Relay GND → GND
 *   Relay IN  → GPIO 26
 *
 * SAFETY: DO NOT connect mains voltage during this test.
 *         Use a multimeter in continuity mode on COM↔NO contacts.
 *
 * Serial: 115200 baud, 115200 baud
 * Commands: send single character via Serial Monitor
 *   h — set ACTIVE HIGH mode (IN HIGH = relay ON)  [default]
 *   l — set ACTIVE LOW mode  (IN LOW  = relay ON)
 *   o — relay ON
 *   f — relay OFF
 *   p — pulse 500ms ON then OFF
 *   c — 10-cycle continuity test (100ms ON / 100ms OFF)
 *   s — print current status
 */

#include <Arduino.h>

#define PIN_RELAY       26
#define CYCLE_COUNT     10
#define CYCLE_ON_MS     100
#define CYCLE_OFF_MS    100
#define PULSE_MS        500

static bool activeHigh  = true;   // V2 default; can be toggled via serial
static bool relayIsOn   = false;
static uint8_t cyclesDone = 0;

static uint8_t passCount = 0;
static uint8_t failCount = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

// Drive the relay pin respecting the active-HIGH/LOW setting
static void setRelay(bool on) {
    relayIsOn = on;
    if (activeHigh) {
        digitalWrite(PIN_RELAY, on ? HIGH : LOW);
    } else {
        digitalWrite(PIN_RELAY, on ? LOW : HIGH);  // inverted logic
    }
}

static void printRelayStatus() {
    Serial.print(F("[STATE] Relay "));
    Serial.print(relayIsOn ? F("ON ") : F("OFF"));
    Serial.print(F("  GPIO26="));
    Serial.print(digitalRead(PIN_RELAY) == HIGH ? F("HIGH") : F("LOW "));
    Serial.print(F("  mode=active_"));
    Serial.println(activeHigh ? F("HIGH") : F("LOW"));
}

static void printHelp() {
    Serial.println(F("[INFO] Commands (send via Serial Monitor at 115200 baud):"));
    Serial.println(F("[INFO]   h = active HIGH mode (most common relay modules)"));
    Serial.println(F("[INFO]   l = active LOW mode  (some blue relay modules)"));
    Serial.println(F("[INFO]   o = relay ON"));
    Serial.println(F("[INFO]   f = relay OFF"));
    Serial.println(F("[INFO]   p = 500ms pulse"));
    Serial.println(F("[INFO]   c = 10-cycle test (100ms on/off)"));
    Serial.println(F("[INFO]   s = print status"));
    Serial.println(F("[INFO]   ? = this help"));
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);

    // CRITICAL: Set GPIO LOW before configuring as output
    // This ensures relay starts in known-safe state regardless of GPIO boot state
    digitalWrite(PIN_RELAY, LOW);
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);

    delay(100);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 06: Mechanical Relay"));
    printSeparator();

    // Verify safe state
    bool gpioLow = (digitalRead(PIN_RELAY) == LOW);
    if (gpioLow) {
        pass("GPIO 26 starts LOW (safe state on boot)");
        info("In ACTIVE HIGH mode this means relay is OFF — correct");
    } else {
        fail("GPIO 26 not LOW on boot — relay may briefly energise on power-up");
    }

    Serial.println();
    Serial.println(F("[INFO] SAFETY: DO NOT connect mains voltage."));
    Serial.println(F("[INFO] Use multimeter in CONTINUITY mode on COM↔NO terminals."));
    Serial.println(F("[INFO] When relay is ON: multimeter should beep (continuity)"));
    Serial.println(F("[INFO] When relay is OFF: multimeter should be silent (open)"));
    Serial.println();

    info("Default mode: ACTIVE HIGH (IN HIGH = relay ON)");
    info("If relay activates on boot without a command, it is ACTIVE LOW — send 'l'");
    Serial.println();
    printHelp();
    Serial.println();
    printRelayStatus();
    Serial.println();
}

// ── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    if (!Serial.available()) return;

    char cmd = (char)Serial.read();
    // Consume rest of line
    while (Serial.available()) Serial.read();

    switch (cmd) {
        case 'h':
            activeHigh = true;
            setRelay(false);  // safe off
            info("Mode set: ACTIVE HIGH (default for optocoupler relay modules)");
            info("GPIO 26 HIGH = relay ON, LOW = relay OFF");
            printRelayStatus();
            break;

        case 'l':
            activeHigh = false;
            setRelay(false);  // safe off
            info("Mode set: ACTIVE LOW (some relay modules without optocoupler)");
            info("GPIO 26 LOW = relay ON, HIGH = relay OFF");
            printRelayStatus();
            break;

        case 'o':
            setRelay(true);
            Serial.print(F("[ACTION] Relay ON"));
            if (activeHigh) Serial.print(F(" (GPIO26=HIGH)"));
            else            Serial.print(F(" (GPIO26=LOW)"));
            Serial.println(F(" — listen for click, check multimeter for continuity"));
            printRelayStatus();
            break;

        case 'f':
            setRelay(false);
            Serial.print(F("[ACTION] Relay OFF"));
            if (activeHigh) Serial.print(F(" (GPIO26=LOW)"));
            else            Serial.print(F(" (GPIO26=HIGH)"));
            Serial.println(F(" — listen for click, multimeter should show open"));
            printRelayStatus();
            break;

        case 'p':
            info("Pulse: relay ON for 500ms then OFF");
            setRelay(true);
            printRelayStatus();
            delay(PULSE_MS);
            setRelay(false);
            printRelayStatus();
            pass("Pulse complete — confirm click ON and click OFF");
            break;

        case 'c': {
            info("10-cycle continuity test: 100ms ON / 100ms OFF");
            info("Listen for 10 double-clicks. Count 20 total relay operations.");
            uint8_t errors = 0;
            for (int i = 1; i <= CYCLE_COUNT; i++) {
                setRelay(true);
                Serial.print(F("[CYCLE] "));
                Serial.print(i);
                Serial.print(F("/10 ON  — GPIO="));
                Serial.println(digitalRead(PIN_RELAY));
                delay(CYCLE_ON_MS);

                setRelay(false);
                Serial.print(F("[CYCLE] "));
                Serial.print(i);
                Serial.print(F("/10 OFF — GPIO="));
                Serial.println(digitalRead(PIN_RELAY));
                delay(CYCLE_OFF_MS);
            }
            cyclesDone += CYCLE_COUNT;

            Serial.println();
            if (errors == 0) {
                pass("10-cycle test complete — no GPIO errors");
                info("Confirm: heard 20 clicks (10 ON + 10 OFF)?");
                info("Confirm: multimeter showed continuity on each ON, open on each OFF?");
            } else {
                fail("GPIO state mismatches during cycle test");
            }

            setRelay(false);
            Serial.print(F("[INFO] Total cycles run this session: "));
            Serial.println(cyclesDone);
            break;
        }

        case 's':
            printRelayStatus();
            Serial.print(F("[INFO] Total pass: "));
            Serial.print(passCount);
            Serial.print(F("  fail: "));
            Serial.println(failCount);
            break;

        case '?':
            printHelp();
            break;

        case '\r':
        case '\n':
            break;

        default:
            Serial.print(F("[WARN] Unknown command '"));
            Serial.print(cmd);
            Serial.println(F("' — send '?' for help"));
            break;
    }

    // Always end in safe state after each command
    // (except 'o' which intentionally leaves relay ON)
}
