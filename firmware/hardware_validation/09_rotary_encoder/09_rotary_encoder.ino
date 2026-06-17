/*
 * TEST 09 — Rotary Encoder (EC11) Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: CW/CCW direction detection, step count accuracy, push-button
 *        press/release detection, hold duration, false-trigger count
 *
 * Wiring:
 *   Encoder VCC (+) → 3V3
 *   Encoder GND (-) → GND
 *   Encoder CLK (A) → GPIO 34  [INPUT-ONLY — NO internal pull-up]
 *   Encoder DT  (B) → GPIO 35  [INPUT-ONLY — NO internal pull-up]
 *   Encoder SW      → GPIO 25  [INPUT_PULLUP available]
 *
 * REQUIRED external pull-ups (if not on module board):
 *   10kΩ from GPIO 34 → 3V3
 *   10kΩ from GPIO 35 → 3V3
 *   (GPIO 25 uses internal pull-up — no external resistor needed)
 *
 * EC11 mechanics: 2 pulses per physical detent step.
 *   This sketch counts HALF-steps (each pulse). Divide by 2 for click count.
 *
 * Serial: 115200 baud
 * Phase 4 hardware — not populated on initial MMS V2 deployment.
 */

#include <Arduino.h>

#define PIN_CLK   34    // Input-only, external pull-up required
#define PIN_DT    35    // Input-only, external pull-up required
#define PIN_SW    25    // Input with internal pull-up

// Debounce
#define BUTTON_DEBOUNCE_MS  20
#define ENCODER_MIN_PULSE_US 1000  // Ignore pulses shorter than this (noise filter)

// Validation targets
#define TARGET_CW_STEPS   10
#define TARGET_CCW_STEPS  10

volatile int32_t  encoderPos       = 0;
volatile int32_t  cwCount          = 0;
volatile int32_t  ccwCount         = 0;
volatile uint32_t falseTriggers    = 0;
volatile bool     newEncoderEvent  = false;
volatile bool     lastEncoderDir   = true;  // true=CW, false=CCW

static bool   lastButtonState     = HIGH;
static bool   stableButtonState   = HIGH;
static uint32_t debounceStart     = 0;
static uint32_t buttonPressTime   = 0;
static uint32_t totalButtonPresses = 0;

static uint8_t passCount = 0;
static uint8_t failCount = 0;

// ── ISR ──────────────────────────────────────────────────────────────────────

// Interrupt on CLK rising edge
// Direction determined by DT state at CLK rising edge:
//   DT HIGH → CW; DT LOW → CCW
void IRAM_ATTR onClkRising() {
    static uint32_t lastPulseUs = 0;
    uint32_t now = micros();

    // Noise filter: ignore if pulse came too quickly
    if ((now - lastPulseUs) < ENCODER_MIN_PULSE_US) {
        falseTriggers++;
        return;
    }
    lastPulseUs = now;

    bool dt = (bool)digitalRead(PIN_DT);
    if (dt) {
        // DT HIGH at CLK rising = Clockwise
        encoderPos++;
        cwCount++;
        lastEncoderDir = true;
    } else {
        // DT LOW at CLK rising = Counter-clockwise
        encoderPos--;
        ccwCount++;
        lastEncoderDir = false;
    }
    newEncoderEvent = true;
}

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }
static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

static void checkPullUps() {
    // With pull-ups in place, pins should read HIGH at rest
    bool clkHigh = (digitalRead(PIN_CLK) == HIGH);
    bool dtHigh  = (digitalRead(PIN_DT)  == HIGH);

    Serial.print(F("[INFO] GPIO 34 (CLK) at rest: "));
    Serial.println(clkHigh ? F("HIGH (pull-up OK)") : F("LOW  — CHECK EXTERNAL PULL-UP"));

    Serial.print(F("[INFO] GPIO 35 (DT ) at rest: "));
    Serial.println(dtHigh  ? F("HIGH (pull-up OK)") : F("LOW  — CHECK EXTERNAL PULL-UP"));

    if (clkHigh && dtHigh) {
        pass("CLK and DT pull-ups confirmed — pins HIGH at rest");
    } else {
        fail("CLK or DT not HIGH at rest — add 10kΩ pull-ups from GPIO 34/35 to 3V3");
        Serial.println(F("[INFO] Without pull-ups: random counting will occur without rotation"));
    }

    bool swHigh = (digitalRead(PIN_SW) == HIGH);
    Serial.print(F("[INFO] GPIO 25 (SW ) at rest: "));
    Serial.println(swHigh ? F("HIGH (internal pull-up OK)") : F("LOW  — button pressed or short"));

    if (swHigh) {
        pass("SW pull-up confirmed — button reads HIGH when released");
    } else {
        Serial.println(F("[WARN] SW reads LOW — button may be pressed, or check wiring"));
    }
}

static void printEncoderStatus() {
    Serial.print(F("[STATE] Position: "));
    Serial.print(encoderPos);
    Serial.print(F("  CW: "));
    Serial.print(cwCount);
    Serial.print(F("  CCW: "));
    Serial.print(ccwCount);
    Serial.print(F("  false: "));
    Serial.print(falseTriggers);
    Serial.print(F("  button_presses: "));
    Serial.println(totalButtonPresses);
}

// ── Button polling (debounced in main loop — not interrupt) ──────────────────

static void pollButton() {
    bool rawState = (bool)digitalRead(PIN_SW);
    uint32_t now  = millis();

    if (rawState != lastButtonState) {
        debounceStart  = now;
        lastButtonState = rawState;
    }

    if ((now - debounceStart) >= BUTTON_DEBOUNCE_MS && rawState != stableButtonState) {
        stableButtonState = rawState;

        if (rawState == LOW) {
            // Pressed
            buttonPressTime = now;
            totalButtonPresses++;
            Serial.println(F("[EVENT] BUTTON PRESSED"));
        } else {
            // Released
            uint32_t held = now - buttonPressTime;
            Serial.print(F("[EVENT] BUTTON RELEASED  held="));
            Serial.print(held);
            Serial.println(F("ms"));

            // Reset encoder position on press
            encoderPos = 0;
            Serial.println(F("[INFO] Position reset to 0"));
        }
    }
}

// ── Validation phases ────────────────────────────────────────────────────────

static void runValidationPhase() {
    static uint8_t phase = 0;
    static bool phasePrinted = false;

    if (!phasePrinted) {
        phasePrinted = true;
        switch (phase) {
            case 0:
                Serial.println();
                Serial.println(F("[PHASE 1] Rotate CW 10 clicks to validate CW detection"));
                break;
            case 1:
                Serial.println();
                Serial.println(F("[PHASE 2] Rotate CCW 10 clicks to validate CCW detection"));
                break;
            case 2:
                Serial.println();
                Serial.println(F("[PHASE 3] Press button 3 times to validate push-button"));
                break;
            case 3:
                // Done
                Serial.println();
                printSeparator();
                Serial.println(F(" VALIDATION SUMMARY"));
                printSeparator();
                printEncoderStatus();
                Serial.println();

                if (cwCount >= TARGET_CW_STEPS)
                    pass("CW rotation detected (>= 10 steps)");
                else {
                    Serial.print(F("[FAIL] CW only ")); Serial.print(cwCount); Serial.println(F(" steps"));
                    failCount++;
                }

                if (ccwCount >= TARGET_CCW_STEPS)
                    pass("CCW rotation detected (>= 10 steps)");
                else {
                    Serial.print(F("[FAIL] CCW only ")); Serial.print(ccwCount); Serial.println(F(" steps"));
                    failCount++;
                }

                if (totalButtonPresses >= 3)
                    pass("Push-button press detected (>= 3 presses)");
                else {
                    Serial.print(F("[FAIL] Only ")); Serial.print(totalButtonPresses); Serial.println(F(" button presses detected"));
                    failCount++;
                }

                if (falseTriggers < 10)
                    pass("False trigger count within tolerance (< 10)");
                else {
                    Serial.print(F("[WARN] "));
                    Serial.print(falseTriggers);
                    Serial.println(F(" false triggers — add pull-ups or reduce encoder cable length"));
                }

                Serial.println();
                Serial.print(F(" STATUS: "));
                Serial.println(failCount == 0 ? F("PASS") : F("FAIL — see above"));
                printSeparator();

                // Reset and loop
                cwCount = 0; ccwCount = 0; falseTriggers = 0; totalButtonPresses = 0;
                phase = 0;
                phasePrinted = false;
                Serial.println(F("[INFO] Counters reset — press reset or continue."));
                return;
        }
    }

    // Phase advancement logic
    switch (phase) {
        case 0: if (cwCount  >= TARGET_CW_STEPS)   { pass("CW phase complete");  phase++; phasePrinted = false; } break;
        case 1: if (ccwCount >= TARGET_CCW_STEPS)   { pass("CCW phase complete"); phase++; phasePrinted = false; } break;
        case 2: if (totalButtonPresses >= 3)         { pass("Button phase complete"); phase++; phasePrinted = false; } break;
    }
}

// ── Setup / Loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 09: Rotary Encoder (EC11)"));
    printSeparator();

    Serial.print(F("[INFO] CLK=GPIO34  DT=GPIO35  SW=GPIO25"));
    Serial.println();
    Serial.println(F("[INFO] GPIO 34/35 are INPUT-ONLY — no internal pull-ups"));
    Serial.println(F("[INFO] External 10kΩ pull-ups required from GPIO 34/35 to 3V3"));
    Serial.println();

    pinMode(PIN_CLK, INPUT);        // INPUT only — no PULLUP available
    pinMode(PIN_DT,  INPUT);        // INPUT only — no PULLUP available
    pinMode(PIN_SW,  INPUT_PULLUP); // Has internal pull-up

    delay(50);  // Settle

    checkPullUps();
    Serial.println();

    // Attach interrupt on CLK rising edge
    attachInterrupt(digitalPinToInterrupt(PIN_CLK), onClkRising, RISING);
    info("Encoder interrupt attached on CLK (GPIO 34) RISING edge");
    Serial.println();
}

void loop() {
    // Process encoder events
    if (newEncoderEvent) {
        noInterrupts();
        bool dir = lastEncoderDir;
        int32_t pos = encoderPos;
        newEncoderEvent = false;
        interrupts();

        Serial.print(F("[EVENT] "));
        Serial.print(dir ? F("CW ") : F("CCW"));
        Serial.print(F("  Position: "));
        Serial.println(pos);
    }

    pollButton();
    runValidationPhase();
    delay(5);
}
