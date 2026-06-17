/**
 * MMS V2 — Kiosk Station Writer
 *
 * Receives JSON card-write commands from the Flask kiosk app via serial,
 * derives the per-card sector key using HMAC-SHA256, and writes:
 *   Block 4 (Sector 1): roll number + schema version
 *   Block 5 (Sector 1): PIN hash + permission bitmask
 *   Block 6 (Sector 1): Reserved (all zeros)
 *   Block 7 (Sector 1): Sector trailer with derived key
 *
 * Commands from host:
 *   "READ_UID\n"   — scan card, return "CARD_UID:<hex>"
 *   <JSON line>   — {"roll":"...","perm_mask":N,"pin_hash":"...","uid":"..."}
 *
 * Responses:
 *   "CARD_UID:<hex>"   — card UID
 *   "AWAITING_CARD"    — write command received, waiting for card placement
 *   "WRITE_SUCCESS"    — card written and verified
 *   "WRITE_FAILED"     — authentication or write error
 *   "ERR:<msg>"        — protocol / hardware error
 */

#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"

// Master key loaded from secrets_station.h (gitignored).
// Copy secrets_station.h.template → secrets_station.h and fill in the key.
#include "secrets_station.h"

#define SS_PIN   5
#define RST_PIN  27

#define SECTOR   1
#define BLOCK4   4
#define BLOCK5   5
#define BLOCK6   6
#define TRAILER7 7

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Pending write state
static bool     pendingWrite = false;
static String   pendingRoll;
static uint32_t pendingPerm;
static char     pendingPinHash[17];  // 16 hex chars for 8 bytes
static String   pendingUid;

// ---------------------------------------------------------------------------
// HMAC-SHA256(master_key, uid_hex_string) → first 6 bytes = sector key
// ---------------------------------------------------------------------------
bool deriveSectorKey(MFRC522::MIFARE_Key* outKey) {
    char uidHex[mfrc522.uid.size * 2 + 1];
    for (int i = 0; i < mfrc522.uid.size; i++) {
        sprintf(&uidHex[i * 2], "%02X", mfrc522.uid.uidByte[i]);
    }

    uint8_t digest[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (mbedtls_md_setup(&ctx, info, 1) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    mbedtls_md_hmac_starts(&ctx, MASTER_KEY, sizeof(MASTER_KEY));
    mbedtls_md_hmac_update(&ctx, (const uint8_t*)uidHex, strlen(uidHex));
    mbedtls_md_hmac_finish(&ctx, digest);
    mbedtls_md_free(&ctx);

    for (int i = 0; i < 6; i++) outKey->keyByte[i] = digest[i];
    return true;
}

// ---------------------------------------------------------------------------
// Authenticate + write one 16-byte block
// ---------------------------------------------------------------------------
bool writeBlock(uint8_t blockAddr, const uint8_t* data, MFRC522::MIFARE_Key& key) {
    uint8_t trailer = ((blockAddr / 4) * 4) + 3;
    MFRC522::StatusCode s = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailer, &key, &mfrc522.uid);
    if (s != MFRC522::STATUS_OK) return false;
    uint8_t buf[16];
    memcpy(buf, data, 16);
    return mfrc522.MIFARE_Write(blockAddr, buf, 16) == MFRC522::STATUS_OK;
}

// ---------------------------------------------------------------------------
// Write sector trailer (updates Key A to the derived key)
// ---------------------------------------------------------------------------
bool writeSectorTrailer(MFRC522::MIFARE_Key& authKey, MFRC522::MIFARE_Key& newKey) {
    MFRC522::StatusCode s = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER7, &authKey, &mfrc522.uid);
    if (s != MFRC522::STATUS_OK) return false;

    uint8_t trailer[16] = {
        newKey.keyByte[0], newKey.keyByte[1], newKey.keyByte[2],
        newKey.keyByte[3], newKey.keyByte[4], newKey.keyByte[5],
        0xFF, 0x07, 0x80, 0x69,           // access bits: Key A has full access
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Key B unused
    };
    return mfrc522.MIFARE_Write(TRAILER7, trailer, 16) == MFRC522::STATUS_OK;
}

// ---------------------------------------------------------------------------
// Block data packers
// ---------------------------------------------------------------------------
void packBlock4(const char* roll, uint8_t* out) {
    memset(out, 0, 16);
    size_t len = strlen(roll);
    if (len > 9) len = 9;
    memcpy(out, roll, len);
    out[9] = 0x02;  // V2 schema
}

void packBlock5(const char* pinHashHex, uint32_t permMask, uint8_t* out) {
    memset(out, 0, 16);
    for (int i = 0; i < 8; i++) {
        char h[3] = {pinHashHex[i*2], pinHashHex[i*2+1], '\0'};
        out[i] = (uint8_t)strtol(h, nullptr, 16);
    }
    // uint32 little-endian at bytes 8-11
    out[8]  =  permMask        & 0xFF;
    out[9]  = (permMask >> 8)  & 0xFF;
    out[10] = (permMask >> 16) & 0xFF;
    out[11] = (permMask >> 24) & 0xFF;
}

// ---------------------------------------------------------------------------
// Full card write + trailer key change + verify
// ---------------------------------------------------------------------------
void executeCardWrite() {
    // Verify UID matches expected
    String actualUid = "";
    for (int i = 0; i < mfrc522.uid.size; i++) {
        char h[3];
        sprintf(h, "%02X", mfrc522.uid.uidByte[i]);
        actualUid += h;
    }

    if (!actualUid.equalsIgnoreCase(pendingUid)) {
        Serial.println("ERR:UID_MISMATCH");
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        pendingWrite = false;
        return;
    }

    // Derive sector key for this card
    MFRC522::MIFARE_Key derivedKey;
    if (!deriveSectorKey(&derivedKey)) {
        Serial.println("ERR:KEY_DERIVATION");
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        pendingWrite = false;
        return;
    }

    // Default (factory) key
    MFRC522::MIFARE_Key defaultKey;
    for (int i = 0; i < 6; i++) defaultKey.keyByte[i] = 0xFF;

    // Determine current key (fresh card = default; re-issue = derived)
    bool freshCard = (mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER7, &defaultKey, &mfrc522.uid) == MFRC522::STATUS_OK);
    mfrc522.PCD_StopCrypto1();
    if (!freshCard) {
        // Try derived key
        if (mfrc522.PCD_Authenticate(
                MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER7, &derivedKey, &mfrc522.uid) != MFRC522::STATUS_OK) {
            Serial.println("ERR:AUTH_FAILED");
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            pendingWrite = false;
            return;
        }
        mfrc522.PCD_StopCrypto1();
    }

    MFRC522::MIFARE_Key& writeKey = freshCard ? defaultKey : derivedKey;

    uint8_t b4[16], b5[16], b6[16];
    packBlock4(pendingRoll.c_str(), b4);
    packBlock5(pendingPinHash, pendingPerm, b5);
    memset(b6, 0, 16);

    if (!writeBlock(BLOCK4, b4, writeKey)) { mfrc522.PCD_StopCrypto1(); Serial.println("ERR:B4"); pendingWrite = false; return; }
    mfrc522.PCD_StopCrypto1();
    if (!writeBlock(BLOCK5, b5, writeKey)) { mfrc522.PCD_StopCrypto1(); Serial.println("ERR:B5"); pendingWrite = false; return; }
    mfrc522.PCD_StopCrypto1();
    if (!writeBlock(BLOCK6, b6, writeKey)) { mfrc522.PCD_StopCrypto1(); Serial.println("ERR:B6"); pendingWrite = false; return; }
    mfrc522.PCD_StopCrypto1();

    // Update sector trailer to use derived key (only on fresh cards)
    if (freshCard) {
        if (!writeSectorTrailer(defaultKey, derivedKey)) {
            mfrc522.PCD_StopCrypto1();
            Serial.println("ERR:TRAILER");
            pendingWrite = false;
            return;
        }
        mfrc522.PCD_StopCrypto1();
    }

    // Verify: re-read Block 4 with derived key; check schema byte
    bool verified = false;
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
            TRAILER7, &derivedKey, &mfrc522.uid) == MFRC522::STATUS_OK) {
        uint8_t rb[18]; uint8_t sz = 18;
        if (mfrc522.MIFARE_Read(BLOCK4, rb, &sz) == MFRC522::STATUS_OK) {
            verified = (rb[9] == 0x02);
        }
        mfrc522.PCD_StopCrypto1();
    }

    Serial.println(verified ? "WRITE_SUCCESS" : "WRITE_FAILED");
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    pendingWrite = false;
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Guard: halt if MASTER_KEY was not replaced (all-zero placeholder)
    bool allZero = true;
    for (size_t i = 0; i < sizeof(MASTER_KEY); i++) {
        if (MASTER_KEY[i] != 0x00) { allZero = false; break; }
    }
    if (allZero) {
        Serial.println("FATAL: MASTER_KEY is all zeros in secrets_station.h");
        Serial.println("Copy secrets_station.h.template to secrets_station.h and fill in the key.");
        while (true) delay(1000);
    }

    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("MMS_WRITER_READY");
}

void loop() {
    // Process one serial command per iteration
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();

        if (line == "READ_UID") {
            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                String uid = "";
                for (int i = 0; i < mfrc522.uid.size; i++) {
                    char h[3]; sprintf(h, "%02X", mfrc522.uid.uidByte[i]);
                    uid += h;
                }
                Serial.println("CARD_UID:" + uid);
                mfrc522.PICC_HaltA();
                mfrc522.PCD_StopCrypto1();
            } else {
                Serial.println("ERR:NO_CARD");
            }
            return;
        }

        // JSON write command
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, line) != DeserializationError::Ok) {
            Serial.println("ERR:JSON_PARSE");
            return;
        }
        const char* roll    = doc["roll"]      | "";
        uint32_t    perm    = doc["perm_mask"]  | 0;
        const char* pinHash = doc["pin_hash"]   | "";
        const char* uid     = doc["uid"]        | "";

        if (!roll[0] || !uid[0] || strlen(pinHash) < 16) {
            Serial.println("ERR:MISSING_FIELDS");
            return;
        }
        pendingRoll  = roll;
        pendingPerm  = perm;
        strncpy(pendingPinHash, pinHash, 16);
        pendingPinHash[16] = '\0';
        pendingUid   = uid;
        pendingWrite = true;
        Serial.println("AWAITING_CARD");
    }

    // Execute pending card write when a card is presented
    if (pendingWrite && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        executeCardWrite();
    }
}
