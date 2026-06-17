#include "card_schema.h"
#include "config.h"
#include <MFRC522.h>
#include "mbedtls/md.h"
#include <string.h>
#include <stdio.h>

extern MFRC522 rfid;

bool CardSchema::deriveSectorKey(const uint8_t* masterKey, const uint8_t* uid, uint8_t uidLen,
                                  uint8_t outKey[6]) {
    // Build uppercase hex string from UID bytes
    char uidHex[uidLen * 2 + 1];
    for (int i = 0; i < uidLen; i++) {
        sprintf(&uidHex[i * 2], "%02X", uid[i]);
    }

    uint8_t digest[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (mbedtls_md_setup(&ctx, info, 1) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    mbedtls_md_hmac_starts(&ctx, masterKey, 32);
    mbedtls_md_hmac_update(&ctx, (const uint8_t*)uidHex, strlen(uidHex));
    mbedtls_md_hmac_finish(&ctx, digest);
    mbedtls_md_free(&ctx);

    memcpy(outKey, digest, 6);
    return true;
}

bool CardSchema::readCard(const uint8_t* masterKey, CardData* out) {
    memset(out, 0, sizeof(CardData));
    out->valid = false;

    uint8_t derivedKeyBytes[6];
    if (!deriveSectorKey(masterKey, rfid.uid.uidByte, rfid.uid.size, derivedKeyBytes)) {
        return false;
    }

    MFRC522::MIFARE_Key key;
    memcpy(key.keyByte, derivedKeyBytes, 6);

    // Authenticate Sector 1 trailer (block 7)
    MFRC522::StatusCode status = rfid.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        CARD_SECTOR_TRAILER,
        &key,
        &rfid.uid
    );
    if (status != MFRC522::STATUS_OK) {
        return false;
    }

    // Read Block 4 (identity)
    uint8_t block4[18];
    uint8_t sz4 = sizeof(block4);
    status = rfid.MIFARE_Read(CARD_BLOCK_IDENTITY, block4, &sz4);
    if (status != MFRC522::STATUS_OK) {
        rfid.PCD_StopCrypto1();
        return false;
    }

    // Check schema version
    if (block4[9] != CARD_SCHEMA_VERSION) {
        rfid.PCD_StopCrypto1();
        return false;
    }
    out->schemaVersion = block4[9];

    // Extract roll number (bytes 0-8, null terminated)
    memcpy(out->rollNumber, block4, 9);
    out->rollNumber[9] = '\0';
    // Trim trailing nulls/spaces
    for (int i = 8; i >= 0; i--) {
        if (out->rollNumber[i] == 0 || out->rollNumber[i] == ' ') out->rollNumber[i] = '\0';
        else break;
    }

    // Read Block 5 (auth + permissions)
    uint8_t block5[18];
    uint8_t sz5 = sizeof(block5);
    status = rfid.MIFARE_Read(CARD_BLOCK_AUTH, block5, &sz5);
    if (status != MFRC522::STATUS_OK) {
        rfid.PCD_StopCrypto1();
        return false;
    }

    memcpy(out->pinHash, block5, 8);

    // uint32 little-endian at bytes 8-11
    out->permMask = (uint32_t)block5[8]
                  | ((uint32_t)block5[9]  << 8)
                  | ((uint32_t)block5[10] << 16)
                  | ((uint32_t)block5[11] << 24);

    rfid.PCD_StopCrypto1();
    out->valid = true;
    return true;
}
