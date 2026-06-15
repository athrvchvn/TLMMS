#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "tlmms";
const char* password = "atharva_is_gay";
const char* mqtt_server = "192.168.0.34";
WiFiClient espClient;
PubSubClient client(espClient);

// MQTT Topics
const char* log_topic = "rfid/access_log";
const char* status_topic = "rfid/status";

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID Configuration
#define RST_PIN    34
#define SS_PIN     17
#define OUTPUT_PIN 26
MFRC522 rfid(SS_PIN, RST_PIN);

// LED Configuration
#define LED_PIN 5
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

// Status LED for WiFi/MQTT
#define WIFI_LED_PIN 2

// Limit Switch Configuration
#define LIMIT_SWITCH_PIN 32
bool cardInserted = false;
bool lastCardInserted = false;

// Timing variables
unsigned long lastLcdUpdate = 0;
unsigned long machineStartTime = 0;
bool machineRunning = false;
bool userAuthorized = false;
String currentRollNumber = "";

// State management variables
bool cardProcessed = false;
unsigned long cardInsertTime = 0;
bool systemReady = true;

// WiFi and MQTT variables
bool wifiConnected = false;
bool mqttConnected = false;
unsigned long lastWifiCheck = 0;
unsigned long lastMqttCheck = 0;
const unsigned long wifiCheckInterval = 10000; // Check WiFi every 10 seconds
const unsigned long mqttCheckInterval = 5000;  // Check MQTT every 5 seconds

// Timing constants
const unsigned long lcdUpdateInterval = 1000;
const unsigned long machineTimeout = 900000; // 15 min
const unsigned long cardDebounceDelay = 200;

// LED variables
unsigned long lastLedUpdate = 0;
bool ledState = false;
const unsigned long ledBlinkInterval = 500;

// RFID Authentication key
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  
  // Initialize hardware components first (core functionality)
  initializeLcd();
  initializeRfid();
  initializeLed();
  initializeAuthKey();
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(WIFI_LED_PIN, OUTPUT);
  
  Serial.println("Core system initialized.");
  
  // Try to initialize WiFi and MQTT (non-blocking for core functionality)
  initializeWifiMqtt();
  
  Serial.println("System ready with MQTT logging support.");
}

void loop() {
  // Core functionality - always runs regardless of WiFi/MQTT status
  handleCardDetection();
  updateLcdDisplay();
  updateLedStatus();
  checkMachineTimeout();
  
  // Non-blocking WiFi and MQTT maintenance
  maintainConnections();
  
  delay(50);
}

// ---------------- CORE RFID FUNCTIONALITY ----------------
void handleCardDetection() {
  // FIXED: Changed from HIGH to LOW for inverted switch logic
  cardInserted = (digitalRead(LIMIT_SWITCH_PIN) == LOW);

  // Card insertion detection
  if (cardInserted && !lastCardInserted) {
    cardInsertTime = millis();
    Serial.println("Card insertion detected...");
  }

  // Process card after debounce
  if (cardInserted && !cardProcessed && systemReady && 
      (millis() - cardInsertTime > cardDebounceDelay)) {
    Serial.println("Card inserted - checking RFID...");
    handleRfidDetection();
    cardProcessed = true;
  }

  // Card removal detection
  if (!cardInserted && lastCardInserted) {
    Serial.println("Card removed - stopping machine...");
    logCardEvent("CARD_REMOVED", currentRollNumber);
    stopMachineCardRemoved();
    cardProcessed = false;
    systemReady = true;
  }

  lastCardInserted = cardInserted;
}

void handleRfidDetection() {
  rfid.PCD_Init();
  delay(100);
  
  int attempts = 0;
  bool cardRead = false;
  
  while (attempts < 5 && !cardRead) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      cardRead = true;
      break;
    }
    delay(100);
    attempts++;
  }
  
  if (cardRead) {
    if (authenticateAndReadCard()) {
      Serial.print("Roll Number: ");
      Serial.println(currentRollNumber);
      
      if (userAuthorized) {
        logCardEvent("ACCESS_GRANTED", currentRollNumber);
        startMachine();
      } else {
        logCardEvent("ACCESS_DENIED", currentRollNumber);
        displayAccessDenied();
      }
    } else {
      logCardEvent("AUTH_ERROR", "UNKNOWN");
      displayAuthError();
    }
    printCardId();
  } else {
    Serial.println("Failed to read RFID card after multiple attempts");
    logCardEvent("READ_ERROR", "UNKNOWN");
    displayAuthError();
  }
}

bool authenticateAndReadCard() {
  byte sector = 1, blockAddr = 5, trailerBlock = 7;
  byte buffer[18];
  byte size = sizeof(buffer);
  MFRC522::StatusCode status;

  userAuthorized = false;
  currentRollNumber = "";

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Authentication failed");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return false;
  }

  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Read failed");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return false;
  }

  // Extract roll number
  for (int i = 0; i < 9 && i < size; i++) {
    if (buffer[i] >= '0' && buffer[i] <= '9') {
      currentRollNumber += (char)buffer[i];
    }
  }

  // Check authorization
  if (size > 9 && buffer[9] == '1') {
    userAuthorized = true;
    Serial.println("User authorized");
  } else {
    userAuthorized = false;
    Serial.println("User not authorized");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

// ---------------- MACHINE CONTROL ----------------
void startMachine() {
  machineRunning = true;
  machineStartTime = millis();
  systemReady = false;
  
  digitalWrite(OUTPUT_PIN, HIGH);
  
  lcd.clear();
  lcd.print("Access Granted");
  lcd.setCursor(0, 1);
  lcd.print("Roll: " + currentRollNumber);
  
  Serial.println("Machine started for user: " + currentRollNumber);
}

void stopMachineCardRemoved() {
  machineRunning = false;
  userAuthorized = false;
  digitalWrite(OUTPUT_PIN, LOW);
  systemReady = true;

  lcd.clear();
  lcd.print("Card Removed");
  lcd.setCursor(0, 1);
  lcd.print("Machine Stopped");
  delay(2000);

  displayCardRemovedMessage();
  Serial.println("Machine stopped - card removed");
  
  currentRollNumber = "";
}

void checkMachineTimeout() {
  if (machineRunning && (millis() - machineStartTime >= machineTimeout)) {
    logCardEvent("SESSION_TIMEOUT", currentRollNumber);
    
    machineRunning = false;
    userAuthorized = false;
    digitalWrite(OUTPUT_PIN, LOW);
    systemReady = true;
    
    lcd.clear();
    lcd.print("Time Up!");
    lcd.setCursor(0, 1);
    lcd.print("Insert Card...");
    
    Serial.println("Machine timeout - session ended");
    currentRollNumber = "";
  }
}

// ---------------- WIFI AND MQTT FUNCTIONS ----------------
void initializeWifiMqtt() {
  // Non-blocking WiFi connection attempt
  Serial.println("Attempting WiFi connection...");
  WiFi.begin(ssid, password);
  
  // Wait maximum 10 seconds for WiFi
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(WIFI_LED_PIN, HIGH);
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Setup MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    connectMqtt();
  } else {
    wifiConnected = false;
    digitalWrite(WIFI_LED_PIN, LOW);
    Serial.println("\nWiFi connection failed. System will work offline.");
  }
}

void maintainConnections() {
  unsigned long now = millis();
  
  // Check WiFi status periodically
  if (now - lastWifiCheck > wifiCheckInterval) {
    lastWifiCheck = now;
    
    if (WiFi.status() == WL_CONNECTED) {
      if (!wifiConnected) {
        wifiConnected = true;
        digitalWrite(WIFI_LED_PIN, HIGH);
        Serial.println("WiFi reconnected!");
      }
    } else {
      if (wifiConnected) {
        wifiConnected = false;
        mqttConnected = false;
        digitalWrite(WIFI_LED_PIN, LOW);
        Serial.println("WiFi disconnected!");
      }
      // Try to reconnect
      WiFi.begin(ssid, password);
    }
  }
  
  // Handle MQTT connection
  if (wifiConnected) {
    if (!client.connected()) {
      if (now - lastMqttCheck > mqttCheckInterval) {
        lastMqttCheck = now;
        connectMqtt();
      }
    } else {
      client.loop(); // Handle MQTT messages
    }
  }
}

void connectMqtt() {
  if (!wifiConnected) return;
  
  Serial.print("Attempting MQTT connection...");
  
  if (client.connect("ESP32_RFID_Logger")) {
    mqttConnected = true;
    Serial.println("MQTT connected!");
    
    // Publish system status
    publishSystemStatus();
    
    // Subscribe to any control topics if needed
    client.subscribe("rfid/control");
    
  } else {
    mqttConnected = false;
    Serial.print("MQTT failed, rc=");
    Serial.print(client.state());
    Serial.println(" will retry later");
  }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  
  // Handle control messages if needed
  if (String(topic) == "rfid/control") {
    // Add control logic here if needed
  }
}

void logCardEvent(String event, String rollNumber) {
  // Always log to Serial for debugging
  Serial.println("Event: " + event + ", Roll: " + rollNumber + ", Time: " + String(millis()));
  
  // Only send MQTT if connected
  if (mqttConnected && client.connected()) {
    // Create JSON log message
    StaticJsonDocument<200> logDoc;
    logDoc["event"] = event;
    logDoc["roll_number"] = rollNumber;
    logDoc["timestamp"] = millis(); // You might want to use real timestamp
    logDoc["device_id"] = "Soldering Station 1"; // Unique device identifier
    
    String logMessage;
    serializeJson(logDoc, logMessage);
    
    // Publish to MQTT
    if (client.publish(log_topic, logMessage.c_str())) {
      Serial.println("MQTT log sent: " + logMessage);
    } else {
      Serial.println("Failed to send MQTT log");
    }
  } else {
    Serial.println("MQTT not connected - log stored locally only");
  }
}

void publishSystemStatus() {
  if (!mqttConnected || !client.connected()) return;
  
  StaticJsonDocument<150> statusDoc;
  statusDoc["device_id"] = "ESP32_RFID_01";
  statusDoc["status"] = "online";
  statusDoc["machine_running"] = machineRunning;
  statusDoc["current_user"] = currentRollNumber;
  statusDoc["timestamp"] = millis();
  
  String statusMessage;
  serializeJson(statusDoc, statusMessage);
  
  client.publish(status_topic, statusMessage.c_str());
}

// ---------------- INITIALIZATION FUNCTIONS ----------------
void initializeLcd() {
  lcd.init();  // Changed from lcd.init() to lcd.begin()
  lcd.backlight();
  lcd.print("Hello User");
  lcd.setCursor(0, 1);
  lcd.print("Tap Card...");
}

void initializeRfid() {
  SPI.begin();
  rfid.PCD_Init();
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
}

void initializeLed() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
}

void initializeAuthKey() {
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

// ---------------- DISPLAY FUNCTIONS ----------------
void displayAccessDenied() {
  lcd.clear();
  lcd.print("Access Denied");
  lcd.setCursor(0, 1);
  lcd.print("Card: " + currentRollNumber);
  Serial.println("Access denied for: " + currentRollNumber);
}

void displayAuthError() {
  lcd.clear();
  lcd.print("Auth Error");
  lcd.setCursor(0, 1);
  lcd.print("Try Again...");
  Serial.println("Authentication error occurred");
}

void displayCardRemovedMessage() {
  lcd.clear();
  lcd.print("Hello User");
  lcd.setCursor(0, 1);
  lcd.print("Tap Card...");
}

// ---------------- LED CONTROL ----------------
void updateLedStatus() {
  if (!machineRunning && !cardInserted && systemReady) {
    blinkRedLed();
  } else if (cardInserted && !userAuthorized && !machineRunning) {
    showRedLed();
  } else if (machineRunning && userAuthorized) {
    showGreenLed();
  } else if (!systemReady || cardInserted) {
    showYellowLed();
  }
}

void blinkRedLed() {
  if (millis() - lastLedUpdate >= ledBlinkInterval) {
    ledState = !ledState;
    leds[0] = ledState ? CRGB(255, 0, 0) : CRGB(0, 0, 0);
    FastLED.show();
    lastLedUpdate = millis();
  }
}

void showRedLed() {
  leds[0] = CRGB(255, 0, 0);
  FastLED.show();
}

void showGreenLed() {
  leds[0] = CRGB(0, 255, 0);
  FastLED.show();
}

void showYellowLed() {
  leds[0] = CRGB(255, 255, 0);
  FastLED.show();
}

// ---------------- LCD UPDATE ----------------
void updateLcdDisplay() {
  if (millis() - lastLcdUpdate >= lcdUpdateInterval) {
    if (machineRunning && userAuthorized) {
      displayMachineTimer();
    }
    lastLcdUpdate = millis();
  }
}

void displayMachineTimer() {
  unsigned long elapsedTime = (millis() - machineStartTime) / 1000;
  unsigned long remainingTime = (machineTimeout / 1000) - elapsedTime;
  
  if (remainingTime > (machineTimeout / 1000)) {
    remainingTime = 0;
  }
  
  unsigned int minutes = remainingTime / 60;
  unsigned int seconds = remainingTime % 60;

  lcd.setCursor(0, 0);
  lcd.print("Roll: " + currentRollNumber);
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
  lcd.print("   ");
}

// ---------------- DEBUG ----------------
void printCardId() {
  Serial.print("Card UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
}