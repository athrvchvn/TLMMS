#include <SPI.h>
#include <MFRC522.h>

// Pin definitions
#define SS_PIN 5
#define RST_PIN 22

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("=== RFID Card Reader Ready ===");
  Serial.println("Place a card on the reader...");
}

void loop() {
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Print UID
  String cardId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardId += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    cardId += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardId.toUpperCase();
  Serial.println("\nCard UID: " + cardId);

  // Authenticate with default key
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Read block 1 (Roll Number)
  String rollNumber = readBlockAsString(1, key);
  // Read block 2 (Machine Flags)
  String machineFlags = readBlockAsString(2, key);

  if (rollNumber != "" && machineFlags != "") {
    Serial.println("Roll Number: " + rollNumber);
    Serial.println("Machine Flags: " + machineFlags);
  } else {
    Serial.println("Error reading card data.");
  }

  // Halt PICC
  mfrc522.PICC_HaltA();
}

String readBlockAsString(byte blockAddr, MFRC522::MIFARE_Key key) {
  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate block
  byte trailerBlock = ((blockAddr / 4) * 4) + 3;
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    trailerBlock,
    &key,
    &(mfrc522.uid)
  );

  if (status != MFRC522::STATUS_OK) {
    Serial.println("Auth failed for block " + String(blockAddr) + ": " + mfrc522.GetStatusCodeName(status));
    return "";
  }

  // Read block
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Read failed for block " + String(blockAddr) + ": " + mfrc522.GetStatusCodeName(status));
    return "";
  }

  // Convert to string (stop at null/empty)
  String result = "";
  for (int i = 0; i < 16; i++) {
    if (buffer[i] != 0) result += (char)buffer[i];
  }

  return result;
}
