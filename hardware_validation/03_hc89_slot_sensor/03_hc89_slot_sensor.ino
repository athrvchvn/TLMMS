/*
 * TEST 03 — HC89 Optical Slot Sensor Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: GPIO read, debounce timing, card-present / card-absent transitions,
 *        false-trigger count over 10 insert/remove cycles
 *
 * Wiring:
 *   HC89 VCC → 3V3
 *   HC89 GND → GND
 *   HC89 OUT → GPIO 32
 *   HC89 A0  → not connected (analog output, unused)
 *
 * Logic: GPIO 32 reads LOW when card is in slot (IR beam blocked)
 *        GPIO 32 reads HIGH when slot is empty (IR beam passes through)
 *
 * Serial: 115200 baud
 */

#include <Arduino.h>

#define PIN_SLOT          32
#define DEBOUNCE_MS       50     // Must match config.h SLOT_DEBOUNCE_MS
#define CYCLE_TARGET      10     // Number of insert/remove cycles to validate

static uint32_t insertCount   = 0;
static uint32_t removeCount   = 0;
static uint32_t falseTriggers = 0;

// Debounced state
static bool     lastStableState = HIGH;  // HIGH = empty
static bool     pendingState    = HIGH;
static uint32_t debounceStart   = 0;
static bool     inDebounce      = false;

// Timing
static uint32_t lastStateChangeMs = 0;
static uint32_t lastPrintMs       = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

static void printStatus() {
    uint32_t now = millis();
    if (now - lastPrintMs < 1000) return;
    lastPrintMs = now;

    Serial.print(F("[STATE] Slot "));
    if (lastStableState == LOW) {
        Serial.print(F("OCCUPIED"));
    } else {
        Serial.print(F("EMPTY   "));
    }
    Serial.print(F("  insertions="));
    Serial.print(insertCount);
    Serial.print(F("  removals="));
    Serial.print(removeCount);
    Serial.print(F("  false="));
    Serial.println(falseTriggers);
}

// ── Debounced read ───────────────────────────────────────────────────────────

// Returns true when a confirmed stable transition occurred
// outState = the NEW stable state after the transition
static bool readDebounced(bool* outState) {
    bool rawState = (bool)digitalRead(PIN_SLOT);
    uint32_t now  = millis();

    if (!inDebounce) {
        // Not currently debouncing — check for a change
        if (rawState != lastStableState) {
            // Start debounce window
            inDebounce    = true;
            pendingState  = rawState;
            debounceStart = now;
        }
        return false;
    }

    // Currently debouncing
    if (rawState != pendingState) {
        // Signal changed again during debounce — reset window
        pendingState  = rawState;
        debounceStart = now;
        falseTriggers++;
        return false;
    }

    if (now - debounceStart >= DEBOUNCE_MS) {
        // Signal stable for DEBOUNCE_MS — accept the transition
        inDebounce       = false;
        lastStableState  = pendingState;
        *outState        = pendingState;
        lastStateChangeMs = now;
        return true;
    }

    return false;
}

// ── Setup ────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 03: HC89 Slot Sensor"));
    printSeparator();

    pinMode(PIN_SLOT, INPUT_PULLUP);
    delay(10);

    Serial.print(F("[INFO] Slot sensor on GPIO "));
    Serial.println(PIN_SLOT);
    info("Logic: GPIO LOW = card in slot, GPIO HIGH = slot empty");
    Serial.print(F("[INFO] Debounce: "));
    Serial.print(DEBOUNCE_MS);
    Serial.println(F(" ms"));
    Serial.println();

    // Read initial state
    bool initState = (bool)digitalRead(PIN_SLOT);
    lastStableState = initState;
    Serial.print(F("[INFO] Initial state: slot "));
    Serial.println(initState == HIGH ? F("EMPTY") : F("OCCUPIED (remove card before test)"));
    Serial.println();

    Serial.print(F("[INFO] Insert and remove a card "));
    Serial.print(CYCLE_TARGET);
    Serial.println(F("× to validate. Summary prints after each cycle."));
    info("Final PASS/FAIL prints after all cycles complete.");
    Serial.println();
}

// ── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    bool newState;
    bool transitionDetected = readDebounced(&newState);

    if (transitionDetected) {
        uint32_t elapsed = millis() - lastStateChangeMs;  // time since previous change

        if (newState == LOW) {
            // Card inserted
            insertCount++;
            Serial.print(F("[EVENT] Card INSERTED  debounce_ok  insertions="));
            Serial.println(insertCount);

        } else {
            // Card removed
            removeCount++;
            Serial.print(F("[EVENT] Card REMOVED   held="));
            Serial.print(elapsed);
            Serial.print(F("ms  removals="));
            Serial.println(removeCount);

            // After each complete cycle, print a summary
            if (removeCount == insertCount) {
                Serial.print(F("[CYCLE] Cycle "));
                Serial.print(removeCount);
                Serial.print(F("/"));
                Serial.print(CYCLE_TARGET);
                Serial.print(F("  false_triggers_this_run="));
                Serial.println(falseTriggers);
            }
        }

        // Final validation after target cycles
        if (removeCount >= CYCLE_TARGET && insertCount >= CYCLE_TARGET) {
            Serial.println();
            printSeparator();
            Serial.println(F(" VALIDATION SUMMARY"));
            printSeparator();
            Serial.print(F("[INFO] Insertions detected : ")); Serial.println(insertCount);
            Serial.print(F("[INFO] Removals detected   : ")); Serial.println(removeCount);
            Serial.print(F("[INFO] False triggers      : ")); Serial.println(falseTriggers);

            if (insertCount == CYCLE_TARGET && removeCount == CYCLE_TARGET) {
                pass("All insert/remove cycles detected correctly");
            } else {
                fail("Cycle count mismatch — sensor missing transitions");
            }

            if (falseTriggers == 0) {
                pass("Zero false triggers — debounce working correctly");
            } else {
                Serial.print(F("[WARN] "));
                Serial.print(falseTriggers);
                Serial.println(F(" false triggers detected — acceptable if < 5"));
                if (falseTriggers < 5) pass("False trigger count within tolerance (< 5)");
                else fail("Too many false triggers — check sensor alignment and debounce");
            }

            Serial.print(F(" STATUS: "));
            uint8_t fails = (insertCount != CYCLE_TARGET || removeCount != CYCLE_TARGET) ? 1 : 0;
            Serial.println(fails == 0 ? F("PASS") : F("FAIL"));
            printSeparator();

            // Reset counters for continued testing
            insertCount   = 0;
            removeCount   = 0;
            falseTriggers = 0;
            Serial.println(F("[INFO] Counters reset — continue testing or press reset."));
            Serial.println();
        }
    }

    printStatus();
    delay(5);
}
