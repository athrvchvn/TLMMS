#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Servo limits
#define SERVOMIN  100
#define SERVOMAX  500

unsigned long startTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);

  Serial.println("PCA9685 READY - Faster Sexy Movement");
  Serial.println("Range: 80° to 130°");

  // Start at center
  moveAllServos(105, 105, 105);
  delay(1000);
  
  startTime = millis();
}

void loop() {
  float t = (millis() - startTime) / 1000.0;   // time in seconds

  // Faster smooth movements (increased speed from previous version)
  float a = 105 + 22 * sin(t * 1.4);           // Servo 0
  float b = 105 + 22 * sin(t * 1.9 + 1.4);     // Servo 1 - different speed + phase
  float c = 105 + 22 * sin(t * 2.6 + 2.8);     // Servo 2 - fastest

  // Extra slow breathing/tilt layer for more organic feel
  float breath = 8 * sin(t * 0.45);

  a += breath;
  b -= breath * 0.8;
  c += breath * 1.1;

  // Constrain safely to new wider range 80-130
  a = constrain(a, 80.0, 130.0);
  b = constrain(b, 80.0, 130.0);
  c = constrain(c, 80.0, 130.0);

  moveAllServos(a, b, c);

  // Optional: Print angles every 400ms
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 400) {
    Serial.printf("Angles: %.1f°  %.1f°  %.1f°\n", a, b, c);
    lastPrint = millis();
  }

  delay(18);   // Fast update rate for smooth & responsive movement
}

// Helper functions
void moveAllServos(float a, float b, float c) {
  pwm.setPWM(0, 0, angleToPulse(a));
  pwm.setPWM(1, 0, angleToPulse(b));
  pwm.setPWM(2, 0, angleToPulse(c));
}

int angleToPulse(float angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}