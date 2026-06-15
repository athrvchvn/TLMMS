#include "display_status.h"
#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// SSD1306 128x32 over I2C
static Adafruit_SSD1306 _oled(128, 32, &Wire, -1);

void DisplayStatus::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!_oled.begin(SSD1306_SWITCHCAPVCC, OLED32_I2C_ADDR)) {
        return;
    }
    _oled.clearDisplay();
    _oled.display();
}

void DisplayStatus::update(bool wifiConnected, bool mqttConnected,
                            const char* machineName, const char* timeStr) {
    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setTextWrap(false);

    // Left: WiFi + MQTT indicators
    // Filled circle = connected, outline = disconnected
    if (wifiConnected) {
        _oled.fillCircle(4, 8, 3, SSD1306_WHITE);
    } else {
        _oled.drawCircle(4, 8, 3, SSD1306_WHITE);
    }
    if (mqttConnected) {
        _oled.fillCircle(13, 8, 3, SSD1306_WHITE);
    } else {
        _oled.drawCircle(13, 8, 3, SSD1306_WHITE);
    }

    // Center: machine name (truncate to ~14 chars at size 1)
    _oled.setCursor(22, 2);
    char name[15];
    strncpy(name, machineName, 14);
    name[14] = '\0';
    _oled.print(name);

    // Right: time or "OFFLINE"
    _oled.setCursor(80, 2);
    _oled.print(timeStr);

    // Bottom row: separator + labels
    _oled.drawFastHLine(0, 16, 128, SSD1306_WHITE);
    _oled.setCursor(0, 20);
    _oled.print(wifiConnected ? "WiFi" : "----");
    _oled.setCursor(30, 20);
    _oled.print(mqttConnected ? "MQTT" : "----");

    _oled.display();
}
