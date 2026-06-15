#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>  

// Pin definitions
#define SS_PIN 5
#define RST_PIN 22
#define BUZZER_PIN 4
#define MOTOR_STEP_PIN 12
#define MOTOR_DIR_PIN 13
#define MOTOR_ENABLE_PIN 14

String pendingWriteData = "";

// Initialize components
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Global variables
String currentRollNumber = "";
String currentCardId = "";
bool cardWriteMode = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_STEP_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, HIGH);

  lcd.setCursor(0, 0);
  lcd.print("RFID Writer Ready");
  lcd.setCursor(0, 1);
  lcd.print("Waiting Serial");
  beep(2);
  delay(2000);
  displayMainMenu();
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

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place card on");
      lcd.setCursor(0, 1);
      lcd.print("reader now...");
      beep(1);
    } else {
      Serial.println("ERR: Invalid input length");
    }
  }

  if ((cardWriteMode || pendingWriteData != "") && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handleCardDetected();
  }
}

void displayMainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready for Card");
  lcd.setCursor(0, 1);
  lcd.print("via Serial");
  Serial.println("=== RFID Ready ===");
}

void handleCardDetected() {
  String cardId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardId += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    cardId += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardId.toUpperCase();
  currentCardId = cardId;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Writing to card");
  Serial.println("Card UID: " + cardId);

  if (pendingWriteData.length() == 14) {
    String rollNumber = pendingWriteData.substring(0, 9);
    String machineFlags = pendingWriteData.substring(9, 14);

    if (writeCardDataDirect(rollNumber, machineFlags)) {
      lcd.setCursor(0, 1);
      lcd.print("Write success");
      beep(3);
      moveMotor(100);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Write failed");
      beep(5);
    }
    pendingWriteData = "";
  }

  cardWriteMode = false;
  mfrc522.PICC_HaltA();
  delay(3000);
  displayMainMenu();
}

bool writeBlock(byte blockAddr, byte* data, MFRC522::MIFARE_Key key) {
  byte trailerBlock = ((blockAddr / 4) * 4) + 3;
  
  Serial.println("=== WRITE BLOCK DEBUG ===");
  Serial.println("Block: " + String(blockAddr) + ", Trailer: " + String(trailerBlock));
  
  // Authenticate
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
    trailerBlock, 
    &key, 
    &(mfrc522.uid)
  );
  
  Serial.println("Auth status: " + String(mfrc522.GetStatusCodeName(status)));
  
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  // Write data
  status = mfrc522.MIFARE_Write(blockAddr, data, 16);
  Serial.println("Write status: " + String(mfrc522.GetStatusCodeName(status)));
  
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  Serial.println("Block " + String(blockAddr) + " write completed");
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
 
 // Prepare roll number block (write to block 1 instead of 4)
 byte rollBlock[16] = {0};  // Initialize all bytes to 0
 rollNumber.getBytes(rollBlock, 16);  // Use full 16 bytes available
 
 Serial.println("Writing roll number to block 1...");
 if (!writeBlock(1, rollBlock, key)) {
   Serial.println("FAILED: Could not write roll number to block 1");
   writeSuccess = false;
 } else {
   Serial.println("SUCCESS: Roll number written to block 1");
 }
 
 // Prepare machine flags block (write to block 2 instead of 5)
 byte flagsBlock[16] = {0};  // Initialize all bytes to 0
 machineFlags.getBytes(flagsBlock, 16);  // Use full 16 bytes available
 
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
   
   // Verify by reading back (optional)
   Serial.println("Verifying write...");
   byte readBuffer[18];
   byte size = sizeof(readBuffer);
   
   if (mfrc522.MIFARE_Read(1, readBuffer, &size) == MFRC522::STATUS_OK) {
     String readRoll = "";
     for (int i = 0; i < 16; i++) {
       if (readBuffer[i] != 0) readRoll += (char)readBuffer[i];
     }
     Serial.println("Verification - Block 1 contains: '" + readRoll + "'");
   }
   
   return true;
 } else {
   Serial.println("=== CARD WRITE FAILED ===");
   return false;
 }
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

void moveMotor(int steps) {
  digitalWrite(MOTOR_ENABLE_PIN, LOW);
  for(int i = 0; i < steps; i++) {
    digitalWrite(MOTOR_STEP_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(MOTOR_STEP_PIN, LOW);
    delayMicroseconds(1000);
  }
  digitalWrite(MOTOR_ENABLE_PIN, HIGH);
}