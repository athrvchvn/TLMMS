/*
 * TEST 01 — ESP32 System Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: Chip ID, CPU frequency, flash size, heap, NVS, LittleFS, onboard LED
 * Wiring: None — plug ESP32 into USB only
 * Serial: 115200 baud
 *
 * Libraries required: None (all built-in to ESP32 Arduino core)
 * Partition scheme: Default 4MB with spiffs (or any scheme with SPIFFS/LittleFS partition)
 */

#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_chip_info.h>

#define PIN_LED       2       // Onboard LED (active HIGH on most NodeMCU boards)
#define NVS_NAMESPACE "mms_config"

static uint8_t passCount = 0;
static uint8_t failCount = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() {
    Serial.println(F("========================================"));
}

static void pass(const char* msg) {
    Serial.print(F("[PASS] "));
    Serial.println(msg);
    passCount++;
}

static void fail(const char* msg) {
    Serial.print(F("[FAIL] "));
    Serial.println(msg);
    failCount++;
}

static void info(const char* msg) {
    Serial.print(F("[INFO] "));
    Serial.println(msg);
}

// ── Test sections ────────────────────────────────────────────────────────────

static void testChipInfo() {
    info("--- Chip Information ---");

    esp_chip_info_t chip;
    esp_chip_info(&chip);

    Serial.print(F("[INFO] Chip model    : "));
    switch (chip.model) {
        case CHIP_ESP32:   Serial.println(F("ESP32")); break;
        case CHIP_ESP32S2: Serial.println(F("ESP32-S2")); break;
        case CHIP_ESP32S3: Serial.println(F("ESP32-S3")); break;
        case CHIP_ESP32C3: Serial.println(F("ESP32-C3")); break;
        default:           Serial.println(F("Unknown")); break;
    }

    Serial.print(F("[INFO] Chip revision : "));
    Serial.println(chip.revision);

    Serial.print(F("[INFO] CPU cores     : "));
    Serial.println(chip.cores);

    Serial.print(F("[INFO] CPU frequency : "));
    Serial.print(getCpuFrequencyMhz());
    Serial.println(F(" MHz"));

    Serial.print(F("[INFO] Flash size    : "));
    uint32_t flashSize = spi_flash_get_chip_size();
    Serial.print(flashSize);
    Serial.print(F(" bytes ("));
    Serial.print(flashSize / (1024 * 1024));
    Serial.println(F(" MB)"));

    Serial.print(F("[INFO] SDK version   : "));
    Serial.println(ESP.getSdkVersion());

    if (getCpuFrequencyMhz() == 240) {
        pass("CPU frequency is 240 MHz");
    } else {
        Serial.print(F("[WARN] CPU frequency is "));
        Serial.print(getCpuFrequencyMhz());
        Serial.println(F(" MHz (expected 240 MHz — check board settings)"));
    }

    if (flashSize >= 4 * 1024 * 1024) {
        pass("Flash size >= 4 MB");
    } else {
        fail("Flash size < 4 MB — upgrade to 4 MB flash module");
    }
}

static void testHeap() {
    info("--- Heap Memory ---");

    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t maxAlloc = ESP.getMaxAllocHeap();

    Serial.print(F("[INFO] Free heap     : "));
    Serial.print(freeHeap);
    Serial.println(F(" bytes"));

    Serial.print(F("[INFO] Min free heap : "));
    Serial.print(minFreeHeap);
    Serial.println(F(" bytes"));

    Serial.print(F("[INFO] Max allocatable block: "));
    Serial.print(maxAlloc);
    Serial.println(F(" bytes"));

    if (freeHeap > 200 * 1024) {
        pass("Free heap > 200 KB");
    } else if (freeHeap > 100 * 1024) {
        Serial.println(F("[WARN] Free heap between 100–200 KB — marginal for V2 firmware"));
    } else {
        fail("Free heap < 100 KB — something is consuming DRAM; full erase and reflash");
    }

    if (maxAlloc > 64 * 1024) {
        pass("Max contiguous block > 64 KB (sufficient for JSON buffers)");
    } else {
        fail("Max contiguous block < 64 KB — heap fragmentation detected");
    }
}

static void testNVS() {
    info("--- NVS (Non-Volatile Storage) ---");

    Preferences prefs;
    bool opened = prefs.begin(NVS_NAMESPACE, false);
    if (opened) {
        pass("NVS namespace \"mms_config\" opened OK (read-write)");

        // Write and read back a test value
        prefs.putUChar("_hw_test", 0xAB);
        uint8_t readback = prefs.getUChar("_hw_test", 0x00);
        if (readback == 0xAB) {
            pass("NVS write/read round-trip OK");
        } else {
            fail("NVS read-back mismatch — flash may be corrupted");
        }
        prefs.remove("_hw_test");
        prefs.end();
    } else {
        fail("NVS namespace open failed — check flash partition table");
    }
}

static void testLittleFS() {
    info("--- LittleFS ---");

    if (!LittleFS.begin(true)) {  // true = format if mount fails
        fail("LittleFS mount failed even after format — check partition scheme");
        return;
    }
    pass("LittleFS mounted OK");

    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();

    Serial.print(F("[INFO] Total bytes   : "));
    Serial.println(total);

    Serial.print(F("[INFO] Used bytes    : "));
    Serial.println(used);

    if (total >= 512 * 1024) {
        pass("LittleFS partition >= 512 KB");
    } else {
        fail("LittleFS partition too small — change partition scheme");
    }

    // Quick write/read test
    File f = LittleFS.open("/hw_test.txt", "w");
    if (f) {
        f.print("MMS_V2_HW_TEST");
        f.close();

        f = LittleFS.open("/hw_test.txt", "r");
        if (f) {
            String content = f.readString();
            f.close();
            if (content == "MMS_V2_HW_TEST") {
                pass("LittleFS write/read round-trip OK");
            } else {
                fail("LittleFS content mismatch on read-back");
            }
            LittleFS.remove("/hw_test.txt");
        } else {
            fail("LittleFS could not re-open test file for reading");
        }
    } else {
        fail("LittleFS could not open test file for writing");
    }
}

static void testLED() {
    info("--- Onboard LED (GPIO 2) ---");
    info("LED should blink 3× now — watch for it");

    pinMode(PIN_LED, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_LED, HIGH);
        delay(200);
        digitalWrite(PIN_LED, LOW);
        delay(200);
    }
    pass("LED blink sequence complete — confirm visually");
}

// ── setup / loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    printSeparator();
    Serial.println(F(" MMS V2 — Test 01: ESP32 System Check"));
    printSeparator();
    Serial.println();

    testChipInfo();
    Serial.println();
    testHeap();
    Serial.println();
    testNVS();
    Serial.println();
    testLittleFS();
    Serial.println();
    testLED();
    Serial.println();

    printSeparator();
    Serial.print(F(" RESULT: "));
    Serial.print(passCount);
    Serial.print(F("/"));
    Serial.print(passCount + failCount);
    Serial.println(F(" passed"));
    Serial.print(F(" STATUS: "));
    Serial.println(failCount == 0 ? F("PASS — ESP32 system is healthy") : F("FAIL — see [FAIL] lines above"));
    printSeparator();
    Serial.println();
}

void loop() {
    static uint32_t lastPrint = 0;
    uint32_t now = millis();
    if (now - lastPrint >= 5000) {
        lastPrint = now;
        Serial.print(F("[LOOP] heap="));
        Serial.print(ESP.getFreeHeap());
        Serial.print(F(" bytes  uptime="));
        Serial.print(now / 1000);
        Serial.println(F("s"));
    }
}
