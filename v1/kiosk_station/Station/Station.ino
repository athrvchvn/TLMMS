#include <SPI.h>
#include <MFRC522.h>

// Pin definitions
#define SS_PIN 5
#define RST_PIN 22

String pendingWriteData = "";

// Initialize RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Global variables
String currentRollNumber = "";
String currentCardId = "";
bool cardWriteMode = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("=== RFID Writer Ready ===");
  Serial.println("Waiting for Serial Data...");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() == 14) {
      String rollNumber = input.substring(0, 9);
      String machineFlags = input.substring(9, 14);
      currentRollNumber = rollNumber;
      pendingWriteData = rollNumber + machineFlags;
      cardWriteMode = true;

      Serial.println("Place card on reader...");
    } else {
      Serial.println("ERR: Invalid input length");
    }
  }

  if ((cardWriteMode || pendingWriteData != "") && 
      mfrc522.PICC_IsNewCardPresent() && 
      mfrc522.PICC_ReadCardSerial()) {
    handleCardDetected();
  }
}

void handleCardDetected() {
  String cardId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardId += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    cardId += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardId.toUpperCase();
  currentCardId = cardId;

  Serial.println("Card UID: " + cardId);

  if (pendingWriteData.length() == 14) {
    String rollNumber = pendingWriteData.substring(0, 9);
    String machineFlags = pendingWriteData.substring(9, 14);

    if (writeCardDataDirect(rollNumber, machineFlags)) {
      Serial.println("Write success");
    } else {
      Serial.println("Write failed");
    }
    pendingWriteData = "";
  }

  cardWriteMode = false;
  mfrc522.PICC_HaltA();
}

bool writeBlock(byte blockAddr, byte* data, MFRC522::MIFARE_Key key) {
  byte trailerBlock = ((blockAddr / 4) * 4) + 3;
  
  // Authenticate
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
    trailerBlock, 
    &key, 
    &(mfrc522.uid)
  );
  
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  // Write data
  status = mfrc522.MIFARE_Write(blockAddr, data, 16);
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  return true;
}

bool writeCardDataDirect(String rollNumber, String machineFlags) {
 Serial.println("=== Starting Card Write Process ===");
 Serial.println("Roll Number: '" + rollNumber + "' (length: " + String(rollNumber.length()) + ")");
 Serial.println("Machine Flags: '" + machineFlags + "' (length: " + String(machineFlags.length()) + ")");
 
 // Default MIFARE key
 MFRC522::MIFARE_Key key;
 for (byte i = 0; i < 6; i++) {
   key.keyByte[i] = 0xFF;
 }
 
 // Check card type first
 MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
 Serial.println("Card Type: " + String(mfrc522.PICC_GetTypeName(piccType)));
 
 if (piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
   Serial.println("ERROR: Unsupported card type for writing");
   return false;
 }
 
 bool writeSuccess = true;
 
 // Prepare roll number block (write to block 1)
 byte rollBlock[16] = {0};
 rollNumber.getBytes(rollBlock, 16);
 
 Serial.println("Writing roll number to block 1...");
 if (!writeBlock(1, rollBlock, key)) {
   Serial.println("FAILED: Could not write roll number to block 1");
   writeSuccess = false;
 } else {
   Serial.println("SUCCESS: Roll number written to block 1");
 }
 
 // Prepare machine flags block (write to block 2)
 byte flagsBlock[16] = {0};
 machineFlags.getBytes(flagsBlock, 16);
 
 Serial.println("Writing machine flags to block 2...");
 if (!writeBlock(2, flagsBlock, key)) {
   Serial.println("FAILED: Could not write machine flags to block 2");
   writeSuccess = false;
 } else {
   Serial.println("SUCCESS: Machine flags written to block 2");
 }
 
 if (writeSuccess) {
   Serial.println("=== CARD WRITE COMPLETED SUCCESSFULLY ===");
   Serial.println("Final Data Written:");
   Serial.println("  Block 1 (Roll): " + rollNumber);
   Serial.println("  Block 2 (Flags): " + machineFlags);
   return true;
 } else {
   Serial.println("=== CARD WRITE FAILED ===");
   return false;
 }
}
