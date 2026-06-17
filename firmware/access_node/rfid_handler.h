#pragma once
#include <stdbool.h>

namespace RFIDHandler {
    void begin();
    bool cardPresent();    // true if MFRC522 sees a new card
    void halt();           // halt + stop crypto after a card interaction
}
