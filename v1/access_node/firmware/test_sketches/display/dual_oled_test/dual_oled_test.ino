#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//
// -------- I2C OLED (128x32) --------
//
#define SCREEN_WIDTH1 128
#define SCREEN_HEIGHT1 32

Adafruit_SSD1306 display1(SCREEN_WIDTH1, SCREEN_HEIGHT1, &Wire, -1);

//
// -------- SPI OLED (128x64) --------
//
#define SCREEN_WIDTH2 128
#define SCREEN_HEIGHT2 64

#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_DC 16
#define OLED_CS 5
#define OLED_RESET 17

Adafruit_SSD1306 display2(SCREEN_WIDTH2, SCREEN_HEIGHT2,
                          &SPI, OLED_DC, OLED_RESET, OLED_CS);

void setup() {

  Serial.begin(115200);

  // Initialize I2C OLED
  Wire.begin(21,22);   // SDA, SCL

  if(!display1.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("I2C OLED not found");
    while(true);
  }

  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(SSD1306_WHITE);

  display1.setCursor(0,0);
  display1.println("I2C OLED");

  display1.setCursor(0,10);
  display1.println("ESP32 Working");

  display1.setCursor(0,20);
  display1.println("128x32 Display");

  display1.display();


  // Initialize SPI OLED
  display2.begin(SSD1306_SWITCHCAPVCC);
  display2.clearDisplay();

  display2.setTextSize(2);
  display2.setTextColor(SSD1306_WHITE);
  display2.setCursor(10,20);
  display2.println("SPI OLED");

  display2.display();
}

void loop() {

}