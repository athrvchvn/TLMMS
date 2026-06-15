#define RELAY_PIN 13   // GPIO connected to relay IN pin

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {

  digitalWrite(RELAY_PIN, HIGH);   // Turn relay ON
  delay(5000);                     // ON for 5 seconds

  digitalWrite(RELAY_PIN, LOW);    // Turn relay OFF
  delay(5000);                     // OFF for 5 seconds

}