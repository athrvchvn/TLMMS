#include "display_primary.h"
#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <stdio.h>

// SSD1306 128x64 over SPI
static Adafruit_SSD1306 _oled(128, 64, &SPI, PIN_OLED64_DC, PIN_OLED64_RST, PIN_OLED64_CS);

static uint8_t   _spinnerFrame  = 0;
static uint32_t  _spinnerLastMs = 0;
static const char* SPINNER_CHARS[] = {"|", "/", "-", "\\"};

static void clearAndSetup() {
    _oled.clearDisplay();
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setTextWrap(false);
}

static void drawHeader(const char* line1, const char* line2 = nullptr) {
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);
    _oled.print(line1);
    if (line2) {
        _oled.setCursor(0, 10);
        _oled.print(line2);
    }
}

static void drawLargeLine(const char* text, int y) {
    _oled.setTextSize(2);
    _oled.setCursor(0, y);
    _oled.print(text);
}

static void formatTime(char* out, uint32_t totalSec) {
    uint32_t h = totalSec / 3600;
    uint32_t m = (totalSec % 3600) / 60;
    uint32_t s = totalSec % 60;
    if (h > 0) snprintf(out, 10, "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    else        snprintf(out, 8,  "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

void DisplayPrimary::begin() {
    if (!_oled.begin(SSD1306_SWITCHCAPVCC)) {
        return;
    }
    _oled.setRotation(0);
    _oled.clearDisplay();
    _oled.display();
}

void DisplayPrimary::showIdle(const char* machineName) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);   _oled.print("Tinkerers' Lab");
    _oled.setCursor(0, 10);  _oled.print("IIT Indore");
    // Divider
    _oled.drawFastHLine(0, 22, 128, SSD1306_WHITE);
    _oled.setTextSize(1);
    _oled.setCursor(0, 26);
    _oled.print(machineName);
    _oled.drawFastHLine(0, 38, 128, SSD1306_WHITE);
    _oled.setTextSize(1);
    _oled.setCursor(10, 50);
    _oled.print("Tap card to start");
    _oled.display();
}

void DisplayPrimary::showCardReading() {
    clearAndSetup();
    _oled.setTextSize(2);
    _oled.setCursor(0, 20);
    _oled.print("Reading");
    _oled.setCursor(0, 40);
    _oled.print("card...");
    _oled.display();
}

void DisplayPrimary::showAccessDenied(const char* reason, const char* roll) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);   _oled.print("!! ACCESS DENIED !!");
    _oled.drawFastHLine(0, 12, 128, SSD1306_WHITE);
    _oled.setCursor(0, 16);  _oled.print(reason);
    _oled.setCursor(0, 32);  _oled.print("Roll:");
    _oled.setCursor(0, 42);  _oled.print(roll);
    _oled.display();
}

void DisplayPrimary::showSessionActive(const char* roll, const char* machineName,
                                        uint32_t elapsedSec, uint32_t limitSec) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);  _oled.print("Roll: ");  _oled.print(roll);
    _oled.setCursor(0, 12); _oled.print(machineName);
    _oled.drawFastHLine(0, 24, 128, SSD1306_WHITE);

    char elapsed[10], limit[10];
    formatTime(elapsed, elapsedSec);
    formatTime(limit,   limitSec);

    _oled.setTextSize(2);
    _oled.setCursor(0, 28);
    _oled.print(elapsed);

    _oled.setTextSize(1);
    _oled.setCursor(0, 52);
    _oled.print("Limit: ");
    _oled.print(limit);
    _oled.display();
}

void DisplayPrimary::showTimeoutWarning(const char* roll, uint32_t remainingSec) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);  _oled.print("!! TIME WARNING !!");
    _oled.setCursor(0, 16);
    char rem[10];
    formatTime(rem, remainingSec);
    _oled.print(rem);
    _oled.print(" remaining");
    _oled.setCursor(0, 32); _oled.print("Remove card soon!");
    _oled.setCursor(0, 48); _oled.print(roll);
    _oled.display();
}

void DisplayPrimary::showSessionEnded(const char* roll, uint32_t durationSec) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);  _oled.print("Session Complete");
    _oled.drawFastHLine(0, 12, 128, SSD1306_WHITE);
    _oled.setCursor(0, 16); _oled.print("Roll: "); _oled.print(roll);
    char dur[10];
    formatTime(dur, durationSec);
    _oled.setCursor(0, 32); _oled.print("Duration: "); _oled.print(dur);
    _oled.setCursor(0, 50); _oled.print("     Thank you!");
    _oled.display();
}

void DisplayPrimary::showMachineDisabled() {
    clearAndSetup();
    _oled.setTextSize(2);
    _oled.setCursor(0, 10); _oled.print("Machine");
    _oled.setCursor(0, 30); _oled.print("Offline");
    _oled.setTextSize(1);
    _oled.setCursor(0, 54); _oled.print("Contact admin");
    _oled.display();
}

void DisplayPrimary::showOtaUpdate(const char* oldVer, const char* newVer, int pct) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);  _oled.print("Updating FW...");
    _oled.setCursor(0, 12); _oled.print(oldVer); _oled.print(" -> "); _oled.print(newVer);
    // Progress bar
    int barW = (128 * pct) / 100;
    _oled.drawRect(0, 28, 128, 12, SSD1306_WHITE);
    _oled.fillRect(0, 28, barW, 12, SSD1306_WHITE);
    char pctStr[8]; snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
    _oled.setCursor(55, 30); _oled.setTextColor(SSD1306_BLACK); _oled.print(pctStr);
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setCursor(0, 50); _oled.print("Do not remove card");
    _oled.display();
}

void DisplayPrimary::showError(const char* code) {
    clearAndSetup();
    _oled.setTextSize(1);
    _oled.setCursor(0, 0);  _oled.print("System Error");
    _oled.setCursor(0, 16); _oled.print(code);
    _oled.setCursor(0, 48); _oled.print("Restarting...");
    _oled.display();
}

void DisplayPrimary::update() {
    // Spinner animation tick — used only during CARD_READING
    if (millis() - _spinnerLastMs >= 200) {
        _spinnerLastMs = millis();
        _spinnerFrame  = (_spinnerFrame + 1) % 4;
    }
}
