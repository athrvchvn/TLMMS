#pragma once
#include <stdint.h>
#include <stdbool.h>

struct CardData {
    char     rollNumber[10];   // 9 digits + null terminator
    uint8_t  pinHash[8];       // HMAC-SHA256(master_key||uid_hex, pin)[0..7]
    uint32_t permMask;         // Machine permissions bitmask (uint32 LE)
    uint8_t  schemaVersion;    // Must be 0x02 for V2
    bool     valid;            // False if parse failed
};

namespace CardSchema {
    /**
     * Derive the 6-byte sector key for the card currently on mfrc522.
     * key = HMAC-SHA256(master_key, uid_hex_string)[0..5]
     */
    bool deriveSectorKey(const uint8_t* masterKey, const uint8_t* uid, uint8_t uidLen,
                         uint8_t outKey[6]);

    /**
     * Read and parse card data from the RFID reader.
     * Authenticates Sector 1 with the derived key, reads blocks 4+5, populates CardData.
     * Returns false if authentication or read fails.
     */
    bool readCard(const uint8_t* masterKey, CardData* out);
}
