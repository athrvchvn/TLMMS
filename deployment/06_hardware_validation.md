# Section 06 — Hardware Validation

← [Hardware Assembly](05_hardware_assembly.md) | → [Raspberry Pi Deployment](07_raspberry_pi_deployment.md)

---

## Purpose

Hardware validation confirms that every component on a freshly assembled node is wired correctly and functional before flashing production firmware. Running these tests takes approximately 2 hours and 15 minutes. A node that passes all 12 tests is ready for provisioning.

**Do not skip this section.** Production firmware cannot distinguish a wiring error from a software bug. Running the validation suite first eliminates hardware as a variable.

---

## Detailed Reference

The complete wiring diagrams, exact expected serial output, pass/fail criteria, and troubleshooting trees for each test are in:

```
v2/hardware_validation/EXECUTION_PLAN.md
```

This section summarises the process and adds the integration test (Test 12) that is not covered in the individual sketches.

---

## Test Suite Overview

| # | Sketch | What it tests | Time | Wiring required |
|---|---|---|---|---|
| 01 | `01_nvs` | NVS read/write/clear | 5 min | None (USB only) |
| 02 | `02_rfid` | MFRC522 SPI, card read, auth | 10 min | MFRC522 |
| 03 | `03_relay` | Relay GPIO, continuity | 5 min | Relay, multimeter |
| 04 | `04_oled_spi` | 128×64 SPI OLED | 5 min | OLED64, MFRC522 |
| 05 | `05_oled_i2c` | 128×32 I2C OLED | 5 min | OLED32 |
| 06 | `06_hc89` | Slot sensor input, debounce | 5 min | HC89 |
| 07 | `07_ws2812` | WS2812 LED | 5 min | WS2812, 470Ω, 100µF |
| 08 | `08_combined_display` | All 3 displays simultaneous | 10 min | OLED64, OLED32, WS2812 |
| 09 | `09_rotary_encoder` | EC11 CLK/DT/SW, interrupts | 10 min | Encoder, 2× 10kΩ |
| 10 | `10_wifi` | WiFi connect, NTP, disconnect | 10 min | Antenna (built-in) |
| 11 | `11_mqtt` | MQTT connect, QoS 0/1, retained | 15 min | RPi running Mosquitto |
| 12 | `12_littlefs` | LittleFS mount/write/rotate | 10 min | None |

---

## Recommended Execution Order

### Phase A — No wiring needed (20 min)
Run these first because they require nothing but a USB cable. If these fail, there is an issue with the ESP32 module itself or the development environment.

1. **Test 01 — NVS** (`01_nvs/01_nvs.ino`)
2. **Test 12 — LittleFS** (`12_littlefs/12_littlefs.ino`)
3. **Test 10 — WiFi** (`10_wifi/10_wifi.ino`) — needs working WiFi AP

### Phase B — Network (15 min, needs RPi)
4. **Test 11 — MQTT** (`11_mqtt/11_mqtt.ino`) — needs Mosquitto running on RPi

### Phase C — I2C bus only (15 min)
5. **Test 05 — OLED I2C** (`05_oled_i2c/05_oled_i2c.ino`)

### Phase D — SPI shared bus (25 min)
6. **Test 04 — OLED SPI** (`04_oled_spi/04_oled_spi.ino`) — wire OLED64
7. **Test 02 — RFID** (`02_rfid/02_rfid.ino`) — add MFRC522 to same SPI bus

### Phase E — Individual GPIO (40 min)
8. **Test 03 — Relay** (`03_relay/03_relay.ino`)
9. **Test 06 — HC89** (`06_hc89/06_hc89.ino`)
10. **Test 07 — WS2812** (`07_ws2812/07_ws2812.ino`)
11. **Test 08 — Combined Display** (`08_combined_display/08_combined_display.ino`)

### Phase F — Interrupt pins (20 min)
12. **Test 09 — Rotary Encoder** (`09_rotary_encoder/09_rotary_encoder.ino`)

---

## Required Arduino IDE Setup

**Board settings (every sketch):**
- Board: ESP32 Dev Module
- Upload Speed: 921600
- CPU Frequency: 240MHz
- Flash Size: 4MB (32Mb)
- Partition Scheme: **Default 4MB with spiffs** (for LittleFS)
- PSRAM: Disabled

**Required libraries (install via Library Manager):**
| Library | Version |
|---|---|
| Adafruit SSD1306 | 2.5.9 |
| Adafruit GFX Library | 1.11.9 |
| MFRC522 | 1.4.11 |
| Adafruit NeoPixel | 1.12.3 |
| PubSubClient | 2.8.0 |
| ArduinoJson | 7.3.1 |
| LittleFS (ESP32) | Built into ESP32 core |

**ESP32 core:** 3.1.x (install via Boards Manager: esp32 by Espressif)

---

## Pass Criteria (Summary)

A node **passes** hardware validation if all of the following are true:

| Test | Pass condition |
|---|---|
| NVS | Round-trip write/read/verify succeeds for all 4 data types |
| RFID | Card UID read matches across multiple reads; auth with default key succeeds |
| Relay | Multimeter confirms continuity on NO terminal during HIGH, open during LOW |
| OLED SPI | `Adafruit SSD1306` text visible on 128×64 display |
| OLED I2C | `Adafruit SSD1306` text visible on 128×32 display |
| HC89 | Serial prints `PRESENT` when card inserted, `ABSENT` when removed; no bounce |
| WS2812 | R/G/B cycle visible; colours match expected (not shifted to GBR) |
| Combined | Both OLEDs + LED all active simultaneously; no I2C/SPI conflict |
| Encoder | CW increments, CCW decrements; SW button detected |
| WiFi | IP assigned; NTP sync; RSSI better than -80 dBm |
| MQTT | QoS 0 round-trip < 50ms; QoS 1 acknowledged; retained cleanup works |
| LittleFS | Mount, write, read, verify, rotate (50KB) all report PASS |

---

## Integration Validation (After All 12 Tests Pass)

After all 12 individual tests pass, run the full-system integration sketch to confirm all hardware works simultaneously — the same way production firmware will use it.

**Sketch:** `v2/hardware_validation/99_full_node/99_full_node.ino`

This sketch:
1. Initialises all hardware in production order
2. Enters an interactive loop:
   - Insert card → MFRC522 reads UID, prints to serial → OLED64 shows UID → LED goes green
   - Remove card → LED goes white, OLED64 shows "Idle"
   - Relay toggles every 5s while card is present
   - OLED32 shows WiFi/MQTT status strip continuously
3. All test outcomes printed to serial: `[PASS]` or `[FAIL]` per component

If this test passes, the node is ready for production firmware.

---

## Recording Results

Use the sign-off log in [EXECUTION_PLAN.md](../hardware_validation/EXECUTION_PLAN.md) to record results for each node.

```
Node: Machine ID [ ]  |  Date: ________  |  Engineer: ____________
01 NVS        [ ] PASS  [ ] FAIL
02 RFID       [ ] PASS  [ ] FAIL
03 Relay      [ ] PASS  [ ] FAIL
04 OLED SPI   [ ] PASS  [ ] FAIL
05 OLED I2C   [ ] PASS  [ ] FAIL
06 HC89       [ ] PASS  [ ] FAIL
07 WS2812     [ ] PASS  [ ] FAIL
08 Combined   [ ] PASS  [ ] FAIL
09 Encoder    [ ] PASS  [ ] FAIL
10 WiFi       [ ] PASS  [ ] FAIL
11 MQTT       [ ] PASS  [ ] FAIL
12 LittleFS   [ ] PASS  [ ] FAIL
99 Integration [ ] PASS  [ ] FAIL
Sign-off: _______________
```

Do not proceed to [Section 11 — Access Node Provisioning](11_access_node_provisioning.md) until the sign-off is complete.
