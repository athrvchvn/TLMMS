#include "rfid_handler.h"
#include "config.h"
#include <SPI.h>
#include <MFRC522.h>

// The global rfid instance used by card_schema.cpp too
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

void RFIDHandler::begin() {
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_RFID_SS);
    rfid.PCD_Init();
}

bool RFIDHandler::cardPresent() {
    return rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
}

void RFIDHandler::halt() {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}
