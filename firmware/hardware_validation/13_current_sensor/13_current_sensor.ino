/**
 * MMS V2 — Test 13: Current Sensor (ADC)
 *
 * Validates that GPIO 36 (ADC1 CH0 / SVP) can be read as an analog input.
 * Prints the raw 12-bit ADC count (0–4095) to Serial at 115200 baud.
 *
 * Hardware required: ESP32, current sensor module wired to GPIO 36, USB cable
 * Hardware optional: Multimeter (verify sensor VCC)
 *
 * No libraries beyond ESP32 Arduino core are needed.
 *
 * Expected output with sensor powered and connected:
 *   A non-zero, stable ADC reading proportional to the measured current.
 *   With no current flowing through the sensor, the reading is typically
 *   around the sensor's quiescent output voltage (e.g. ~2048 for ACS712 at
 *   zero current on a 3.3V supply). Exact value depends on the sensor model.
 *
 * Future processing (calibration, filtering, amp conversion) will be added
 * to firmware/access_node/current_sensor.cpp — not here.
 */

#define PIN_CURRENT_SENSOR  36   // ADC1 CH0 (SVP) — input-only pin

#define SAMPLE_INTERVAL_MS  500  // Print rate
#define PRINT_HEADER_EVERY  20   // Reprint column header every N rows

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("========================================"));
    Serial.println(F(" MMS V2 — Test 13: Current Sensor (ADC)"));
    Serial.println(F("========================================"));
    Serial.println(F("[INFO] GPIO 36 = ADC1 CH0 (SVP) — input-only"));
    Serial.println(F("[INFO] ADC resolution: 12-bit (0–4095)"));
    Serial.println(F("[INFO] ADC range: 0–3.3V (default 11dB attenuation)"));
    Serial.println(F("[INFO] No processing applied — raw counts only"));
    Serial.println();

    // GPIO 36 is input-only; pinMode INPUT documents intent
    pinMode(PIN_CURRENT_SENSOR, INPUT);

    // Phase 1: confirm pin is readable (any non-stuck value)
    Serial.println(F("[INFO] === Phase 1: Single-shot read ==="));
    uint16_t sample = (uint16_t)analogRead(PIN_CURRENT_SENSOR);
    Serial.print(F("[INFO] GPIO 36 raw ADC = "));
    Serial.println(sample);

    if (sample == 0 || sample == 4095) {
        Serial.println(F("[WARN] ADC reads 0 or 4095 — sensor may be disconnected or supply missing"));
        Serial.println(F("[WARN] Continuing to continuous monitor — check wiring"));
    } else {
        Serial.println(F("[PASS] ADC reads non-saturated value"));
    }

    Serial.println();
    Serial.println(F("[INFO] === Phase 2: Continuous monitor (Ctrl+C or reset to stop) ==="));
    Serial.println(F("[INFO] Expected: stable non-zero reading when sensor is powered"));
    Serial.println();
}

static uint32_t _lastPrintMs   = 0;
static uint32_t _sampleCount   = 0;
static uint16_t _minSeen       = 4095;
static uint16_t _maxSeen       = 0;

void loop() {
    if (millis() - _lastPrintMs < SAMPLE_INTERVAL_MS) return;
    _lastPrintMs = millis();

    uint16_t raw = (uint16_t)analogRead(PIN_CURRENT_SENSOR);
    _sampleCount++;
    if (raw < _minSeen) _minSeen = raw;
    if (raw > _maxSeen) _maxSeen = raw;

    // Reprint header periodically for readability
    if (_sampleCount % PRINT_HEADER_EVERY == 1) {
        Serial.println(F("  #     raw    min   max  uptime_s"));
        Serial.println(F("  ----  -----  ----  ----  --------"));
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "  %4lu  %5u  %4u  %4u  %8lu",
             (unsigned long)_sampleCount,
             raw, _minSeen, _maxSeen,
             (unsigned long)(millis() / 1000));
    Serial.println(buf);
}
