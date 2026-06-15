/*
 * TEST 02 — MFRC522 RFID Reader Validation
 * MMS V2 Hardware Validation Package
 *
 * Tests: SPI communication, firmware version, self-test, card UID read,
 *        Sector 0 Block 0 authentication with default key, raw block read
 *
 * Wiring:
 *   MFRC522 SDA  → GPIO 5
 *   MFRC522 SCK  → GPIO 18
 *   MFRC522 MOSI → GPIO 23
 *   MFRC522 MISO → GPIO 19
 *   MFRC522 RST  → GPIO 27
 *   MFRC522 3.3V → 3V3   (NOT 5V — will damage module)
 *   MFRC522 GND  → GND
 *   MFRC522 IRQ  → not connected
 *
 * Required library: MFRC522 by GithubCommunity (Library Manager)
 * Serial: 115200 baud
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define PIN_RFID_SS   5
#define PIN_RFID_RST  27
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23

MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

static uint8_t passCount = 0;
static uint8_t failCount = 0;
static uint32_t cardCount = 0;

// ── Helpers ─────────────────────────────────────────────────────────────────

static void printSeparator() { Serial.println(F("========================================")); }

static void pass(const char* msg) { Serial.print(F("[PASS] ")); Serial.println(msg); passCount++; }
static void fail(const char* msg) { Serial.print(F("[FAIL] ")); Serial.println(msg); failCount++; }
static void info(const char* msg) { Serial.print(F("[INFO] ")); Serial.println(msg); }

static void printHex(const byte* buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
}

static const char* piccTypeName(MFRC522::PICC_Type t) {
    switch (t) {
        case MFRC522::PICC_TYPE_MIFARE_MINI: return "MIFARE Mini (320B)";
        case MFRC522::PICC_TYPE_MIFARE_1K:   return "MIFARE Classic 1K";
        case MFRC522::PICC_TYPE_MIFARE_4K:   return "MIFARE Classic 4K";
        case MFRC522::PICC_TYPE_MIFARE_UL:   return "MIFARE Ultralight";
        case MFRC522::PICC_TYPE_ISO_14443_4: return "ISO 14443-4";
        case MFRC522::PICC_TYPE_UNKNOWN:     return "Unknown";
        default:                              return "Other";
    }
}

// ── Init phase ───────────────────────────────────────────────────────────────

static bool initPhase() {
    printSeparator();
    Serial.println(F(" MMS V2 — Test 02: MFRC522 RFID Reader"));
    printSeparator();

    Serial.print(F("[INFO] SPI init: SCK="));
    Serial.print(PIN_SPI_SCK);
    Serial.print(F(" MISO="));
    Serial.print(PIN_SPI_MISO);
    Serial.print(F(" MOSI="));
    Serial.println(PIN_SPI_MOSI);

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_RFID_SS);
    rfid.PCD_Init();
    delay(50);

    // Read firmware version — 0x91=v1.0, 0x92=v2.0, 0x00/0xFF=no response
    byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.print(F("[INFO] MFRC522 firmware version: 0x"));
    Serial.print(version, HEX);

    if (version == 0x91) {
        Serial.println(F(" (v1.0)"));
        pass("MFRC522 detected — firmware v1.0");
    } else if (version == 0x92) {
        Serial.println(F(" (v2.0)"));
        pass("MFRC522 detected — firmware v2.0");
    } else if (version == 0x00 || version == 0xFF) {
        Serial.println(F(" — NO RESPONSE"));
        fail("MFRC522 not responding — check SPI wiring (SDA/SCK/MOSI/MISO/RST and 3.3V)");
        return false;
    } else {
        Serial.println(F(" (unknown version — module may be counterfeit)"));
        pass("MFRC522 responded (non-standard version)");
    }

    // Self-test
    Serial.print(F("[INFO] Running MFRC522 self-test... "));
    if (rfid.PCD_PerformSelfTest()) {
        Serial.println(F("OK"));
        pass("MFRC522 self-test passed");
    } else {
        Serial.println(F("FAILED"));
        fail("MFRC522 self-test failed — module damaged or counterfeit");
    }

    // Re-init after self-test (self-test resets registers)
    rfid.PCD_Init();
    rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
    delay(50);

    return (failCount == 0);
}

// ── Card reading ─────────────────────────────────────────────────────────────

static void processCard() {
    cardCount++;

    Serial.println();
    Serial.print(F("[INFO] Card #"));
    Serial.print(cardCount);
    Serial.println(F(" detected!"));

    // UID
    Serial.print(F("[INFO] UID length : "));
    Serial.print(rfid.uid.size);
    Serial.println(F(" bytes"));

    Serial.print(F("[INFO] UID        : "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    if (rfid.uid.size < 4) {
        fail("UID too short — expected 4+ bytes for MIFARE Classic");
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return;
    }
    pass("UID read OK (4+ bytes)");

    // Card type
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.print(F("[INFO] PICC type  : "));
    Serial.println(piccTypeName(piccType));

    if (piccType == MFRC522::PICC_TYPE_MIFARE_1K) {
        pass("Card type is MIFARE Classic 1K (correct for MMS V2)");
    } else if (piccType == MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("[WARN] MIFARE Classic 4K — works with MMS V2 but not standard"));
    } else {
        Serial.print(F("[WARN] Card type "));
        Serial.print(piccTypeName(piccType));
        Serial.println(F(" — MMS V2 requires MIFARE Classic 1K"));
    }

    // Authenticate Sector 0 with default key
    MFRC522::MIFARE_Key defaultKey;
    for (int i = 0; i < 6; i++) defaultKey.keyByte[i] = 0xFF;

    Serial.print(F("[INFO] Authenticating Sector 0, Block 0 with default key (FF×6)..."));
    MFRC522::StatusCode status = rfid.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, 3, &defaultKey, &rfid.uid);

    if (status == MFRC522::STATUS_OK) {
        Serial.println(F(" OK"));
        pass("Sector 0 authenticated with default key");

        // Read Block 0 (manufacturer block)
        byte blockData[18];
        byte blockLen = 18;
        status = rfid.MIFARE_Read(0, blockData, &blockLen);

        if (status == MFRC522::STATUS_OK) {
            Serial.print(F("[DATA] Block 0 raw : "));
            printHex(blockData, 16);
            Serial.println();

            bool allZero = true;
            for (int i = 0; i < 16; i++) if (blockData[i] != 0) { allZero = false; break; }

            if (!allZero) {
                pass("Block 0 read OK (contains manufacturer data)");
            } else {
                Serial.println(F("[WARN] Block 0 is all zeros — this card may be blank or non-standard"));
            }
        } else {
            Serial.print(F("[FAIL] Block 0 read failed: "));
            Serial.println(rfid.GetStatusCodeName(status));
            failCount++;
        }
    } else {
        Serial.println();
        Serial.print(F("[FAIL] Authentication failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        Serial.println(F("[INFO] This may mean Sector 0 Key A was changed by a previous write."));
        Serial.println(F("[INFO] Try a fresh (unwritten) MIFARE Classic 1K card."));
        failCount++;
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    Serial.println(F("[INFO] Card test complete — remove card and tap again, or wait."));
    Serial.print(F("[INFO] Total cards tested so far: "));
    Serial.println(cardCount);
}

// ── setup / loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    if (!initPhase()) {
        Serial.println(F("[FATAL] Cannot proceed — check SPI wiring and retry."));
        while (true) delay(1000);
    }

    Serial.println();
    info("Waiting for MIFARE Classic 1K card... (hold within 3 cm of antenna)");
    info("Press reset to re-run init phase.");
    Serial.println();
}

void loop() {
    // Debounce: wait a bit before next scan attempt
    delay(100);

    if (!rfid.PICC_IsNewCardPresent()) return;
    if (!rfid.PICC_ReadCardSerial())   return;

    processCard();
    delay(1000);  // Brief pause before scanning again
}
