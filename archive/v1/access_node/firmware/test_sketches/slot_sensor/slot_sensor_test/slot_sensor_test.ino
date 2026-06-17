#define SLOT_SENSOR 12   // signal pin

void setup() {
  Serial.begin(115200);
  pinMode(SLOT_SENSOR, INPUT);
}

void loop() {

  int sensorState = digitalRead(SLOT_SENSOR);

  if(sensorState == HIGH)
  {
    Serial.println("Object Detected (slot blocked)");
  }
  else
  {
    Serial.println("No Object");
  }

  delay(200);
}