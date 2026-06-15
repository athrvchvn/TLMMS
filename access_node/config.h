#pragma once

// ---------------------------------------------------------------------------
// CANONICAL MACHINE ID TABLE
// These IDs MUST be consistent across three places:
//   1. This file (MACHINE_ID per node)
//   2. kiosk_station/config.py  MACHINE_IDS dict values
//   3. Firestore /machines/{id} document IDs
//
//   ID | Machine Name
//   ---+------------------------
//    1 | Creality 3D Printer
//    2 | Fracktal Laser Cutter
//    3 | Soldering Station 1
//    4 | Soldering Station 2
//    5 | Bosche Grinder
//
// Before flashing a node: set MACHINE_ID and MACHINE_NAME to the correct
// row from the table above. Mismatch = wrong permission bit checked = access
// denied for everyone, or access granted to the wrong machine.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Per-node configuration — edit before flashing each node
// ---------------------------------------------------------------------------

#define MACHINE_ID    1                          // 1–32; must match Firestore /machines/{id}
#define MACHINE_NAME  "Creality 3D Printer"      // Must match the name for MACHINE_ID above
#define SESSION_LIMIT_MIN 15                     // Default session limit in minutes

// ---------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------
#define MQTT_BROKER   "192.168.0.10"             // Raspberry Pi static IP
#define MQTT_PORT     1883

// ---------------------------------------------------------------------------
// Hardware pin assignments
// ---------------------------------------------------------------------------

// MFRC522 RFID reader (SPI)
#define PIN_RFID_SS   5
#define PIN_RFID_RST  27   // NOT GPIO 34 (input-only); was wrong in V1

// SPI bus (shared)
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23

// 128x64 OLED (SPI — primary display)
#define PIN_OLED64_DC   17
#define PIN_OLED64_CS   16
#define PIN_OLED64_RST  4

// 128x32 OLED (I2C — status strip)
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22
#define OLED32_I2C_ADDR 0x3C

// Relay / SSR
#define PIN_RELAY     26

// HC89 slot sensor (card present = LOW with INPUT_PULLUP)
#define PIN_SLOT      32

// WS2812 RGB LED (optional, deferred to Phase 3)
#define PIN_LED       33
#define LED_COUNT     1

// Onboard LED (WiFi status indicator)
#define PIN_WIFI_LED  2

// ---------------------------------------------------------------------------
// Tuning
// ---------------------------------------------------------------------------
#define SLOT_DEBOUNCE_MS      50
#define RFID_READ_TIMEOUT_MS  2000
#define SESSION_WARNING_SEC   120    // 2 minutes before limit
#define WATCHDOG_TIMEOUT_SEC  30
#define MQTT_RECONNECT_DELAY_MS 5000
#define LOG_ROTATE_BYTES      51200  // 50 KB
#define HEARTBEAT_INTERVAL_MS 30000
#define NTP_SYNC_INTERVAL_MS  3600000  // 1 hour

// ---------------------------------------------------------------------------
// RFID card schema
// ---------------------------------------------------------------------------
#define CARD_SCHEMA_VERSION   0x02
#define CARD_SECTOR           1
#define CARD_BLOCK_IDENTITY   4
#define CARD_BLOCK_AUTH       5
#define CARD_BLOCK_RESERVED   6
#define CARD_SECTOR_TRAILER   7

// ---------------------------------------------------------------------------
// Firmware version
// ---------------------------------------------------------------------------
#define FW_VERSION  "2.0.0"
