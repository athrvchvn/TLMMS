# MMS V2 — Hardware Validation Guide

## Purpose

Validate every hardware component independently before running integrated V2 firmware.
A junior engineer should be able to validate an entire node using this guide alone.

## Validation Order

Run tests in this order. Later tests depend on earlier ones passing.

| Step | Test | Hardware Required | Prereqs |
|------|------|-------------------|---------|
| 01 | ESP32 System | ESP32 only | None |
| 02 | MFRC522 RFID | ESP32 + MFRC522 + MIFARE card | 01 |
| 03 | HC89 Slot Sensor | ESP32 + HC89 | 01 |
| 04 | 128×64 OLED (SPI) | ESP32 + SSD1306 SPI | 01 |
| 05 | 128×32 OLED (I2C) | ESP32 + SSD1306 I2C | 01 |
| 06 | Mechanical Relay | ESP32 + relay module + multimeter | 01 |
| 07 | Solid State Relay | ESP32 + SSR module + multimeter | 01 |
| 08 | WS2812 LED | ESP32 + WS2812 module | 01 |
| 09 | Rotary Encoder | ESP32 + EC11 encoder | 01 |
| 10 | WiFi | ESP32 + lab network | 01 |
| 11 | MQTT | ESP32 + broker IP | 10 |
| 12 | LittleFS | ESP32 only | 01 |

## Required Equipment

- ESP32 NodeMCU 38-pin development board
- USB-A to Micro-USB cable (data capable, not charge-only)
- Multimeter with continuity and AC/DC voltage modes
- Breadboard and jumper wires
- MIFARE Classic 1K card (for MFRC522 test)
- Arduino IDE 2.x or PlatformIO

## Required Libraries (Arduino IDE — Library Manager)

| Library | Author | Version | Used by |
|---------|--------|---------|---------|
| MFRC522 | GithubCommunity | ≥1.4.10 | Test 02 |
| Adafruit SSD1306 | Adafruit | ≥2.5.7 | Tests 04, 05 |
| Adafruit GFX Library | Adafruit | ≥1.11.9 | Tests 04, 05 |
| Adafruit NeoPixel | Adafruit | ≥1.12.0 | Test 08 |
| PubSubClient | Nick O'Leary | ≥2.8.0 | Test 11 |
| ArduinoJson | Benoit Blanchon | ≥7.0.0 | Test 11 |

ESP32 board package: `esp32 by Espressif Systems` ≥3.0.0
Install via: Boards Manager → search "esp32"

## Global Pin Reference

All GPIO numbers here are fixed in V2. Do not remap without updating both `config.h`
and the relevant integration module.

| GPIO | Function | Module | Direction | Notes |
|------|----------|--------|-----------|-------|
| 2 | Onboard LED / WiFi status | Built-in | OUTPUT | Active HIGH |
| 4 | OLED64 RST | 128×64 SPI OLED | OUTPUT | Active LOW |
| 5 | RFID SS (CS) | MFRC522 | OUTPUT | Active LOW |
| 16 | OLED64 CS | 128×64 SPI OLED | OUTPUT | Active LOW |
| 17 | OLED64 DC | 128×64 SPI OLED | OUTPUT | HIGH=data, LOW=cmd |
| 18 | SPI SCK | MFRC522 + OLED64 | OUTPUT | Shared SPI bus |
| 19 | SPI MISO | MFRC522 | INPUT | Shared SPI bus |
| 21 | I2C SDA | 128×32 I2C OLED | Bidirectional | Pull-up to 3.3V |
| 22 | I2C SCL | 128×32 I2C OLED | OUTPUT | Pull-up to 3.3V |
| 23 | SPI MOSI | MFRC522 + OLED64 | OUTPUT | Shared SPI bus |
| 25 | Rotary encoder SW | EC11 | INPUT_PULLUP | Phase 4 |
| 26 | Relay / SSR control | Relay module | OUTPUT | Polarity via config |
| 27 | RFID RST | MFRC522 | OUTPUT | Active LOW |
| 32 | HC89 slot sensor | HC89 module | INPUT_PULLUP | LOW = card present |
| 33 | WS2812 data | LED module | OUTPUT | Phase 3 |
| 34 | Rotary encoder CLK | EC11 | INPUT | Input-only, no pull-up |
| 35 | Rotary encoder DT | EC11 | INPUT | Input-only, no pull-up |

GPIO 34, 35, 36, 39 are input-only — no internal pull-ups available on these pins.

---

## Test 01 — ESP32 System

### Wiring Diagram

None required. Plug ESP32 into USB only.

### Pin Mapping Table

| Signal | GPIO | Notes |
|--------|------|-------|
| USB Serial | — | via CP2102 / CH340 on board |
| Onboard LED | 2 | used as blink-alive indicator |

### Expected Serial Output (115200 baud)

```
========================================
 MMS V2 — Test 01: ESP32 System Check
========================================
[INFO] Chip model    : ESP32-D0WDQ6
[INFO] Chip revision : 3
[INFO] CPU frequency : 240 MHz
[INFO] Flash size    : 4194304 bytes (4 MB)
[INFO] Flash speed   : 80 MHz
[INFO] PSRAM size    : 0 bytes (no PSRAM on most NodeMCU variants)
[INFO] Free heap     : 303120 bytes
[INFO] Min free heap : 296412 bytes
[INFO] SDK version   : v5.x.x
[PASS] Heap > 200 KB
[INFO] NVS init...
[PASS] NVS namespace "mms_config" opened OK
[INFO] LittleFS init...
[PASS] LittleFS mounted — total: 1458176 bytes, used: 0 bytes
[INFO] Onboard LED blink test — LED should flash 3× now
[PASS] LED blink complete
========================================
 RESULT: 3/3 passed
 STATUS: PASS — ESP32 system is healthy
========================================
Loop: heap = 301XX bytes, uptime = Xs
```

### Pass/Fail Criteria

- [ ] Chip model is `ESP32-D0WDQ6` or `ESP32-D0WD-V3`
- [ ] CPU frequency is 240 MHz
- [ ] Flash size ≥ 4 MB
- [ ] Free heap > 200 KB
- [ ] NVS namespace opens without error
- [ ] LittleFS mounts without error
- [ ] Onboard LED (GPIO 2) blinks 3×

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| No serial output | Wrong COM port, or charge-only USB cable |
| `Brownout detector triggered` reset loop | Insufficient USB power; try a different cable or powered hub |
| `Flash read err` | Corrupt flash; erase and reflash bootloader |
| LittleFS mount fails first time | Normal — first use formats the partition. Should pass on second boot. |
| Heap < 100 KB | Other code left in flash consuming DRAM; full erase and reflash |

### Troubleshooting

1. **No serial output:** Arduino IDE → Tools → Port. If port not listed, install CP2102 or CH340 driver.
2. **Brownout:** Swap USB cable. Brownout usually means the cable drops >0.5V under load.
3. **Flash error:** Tools → Erase All Flash Before Sketch Upload → enable.
4. **LittleFS always fails:** Ensure `Partition Scheme` in Arduino IDE → Tools is set to `Default 4MB with spiffs` or `Huge APP (3MB No OTA/1MB SPIFFS)`. The partition scheme must allocate a LittleFS/SPIFFS partition.

---

## Test 02 — MFRC522 RFID Reader

### Wiring Diagram

```
    ESP32 NodeMCU                    MFRC522 Module
    ┌──────────────┐                 ┌─────────────┐
    │          3V3 │────────────────►│ 3.3V        │
    │          GND │────────────────►│ GND         │
    │       GPIO 5 │────────────────►│ SDA (SS/CS) │
    │      GPIO 18 │────────────────►│ SCK         │
    │      GPIO 23 │────────────────►│ MOSI        │
    │      GPIO 19 │◄────────────────│ MISO        │
    │      GPIO 27 │────────────────►│ RST         │
    │              │                 │ IRQ  (leave │
    └──────────────┘                 │       open) │
                                     └─────────────┘

IMPORTANT: MFRC522 is a 3.3V module. Do NOT connect VCC to 5V — it will damage the chip.
```

### Pin Mapping Table

| MFRC522 Pin | ESP32 GPIO | Function |
|-------------|------------|----------|
| SDA (SS) | 5 | SPI chip select |
| SCK | 18 | SPI clock |
| MOSI | 23 | SPI data to MFRC522 |
| MISO | 19 | SPI data from MFRC522 |
| RST | 27 | Reset (active LOW) |
| 3.3V | 3V3 | Power |
| GND | GND | Ground |
| IRQ | — | Not connected |

### Expected Serial Output

Phase A — Init (with module connected):
```
========================================
 MMS V2 — Test 02: MFRC522 RFID Reader
========================================
[INFO] SPI init on SCK=18 MISO=19 MOSI=23
[INFO] MFRC522 init on SS=5 RST=27
[PASS] Firmware version: 0x92 (v2.0) — module present and responding
[INFO] Running MFRC522 self-test...
[PASS] Self-test: OK
[INFO] Waiting for card... (tap a MIFARE Classic 1K card)
```

Phase B — Card detected:
```
[INFO] Card detected!
[INFO] UID length : 4 bytes
[INFO] UID        : A1 B2 C3 D4
[INFO] PICC type  : MIFARE Classic 1K
[INFO] Authenticating Sector 0 Block 0 with default key FF FF FF FF FF FF...
[PASS] Authentication OK
[INFO] Block 0 raw data:
[DATA] XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
[PASS] Block read OK (manufacturer block — data expected to be non-zero)
[INFO] Halting card...
[PASS] Card test complete

[INFO] Waiting for next card...
```

Phase A — No module / SPI problem:
```
[FAIL] Firmware version: 0x00 — no response. Check wiring. Aborting.
```

### Expected OLED Output

This test outputs to serial only. No OLED required.

### Pass/Fail Criteria

- [ ] Firmware version is `0x91` (v1.0) or `0x92` (v2.0) — any other value means no response
- [ ] Self-test returns OK
- [ ] Card UID reads as 4 bytes (MIFARE Classic 1K)
- [ ] Sector 0 Block 0 authenticates with default key `FF FF FF FF FF FF`
- [ ] Block 0 read returns 16 bytes (non-all-zeros, as Block 0 is manufacturer data)
- [ ] Card halts cleanly (no `STATUS_ERROR`)

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| Firmware version `0x00` or `0xFF` | SPI wiring error; check MISO/MOSI/SCK/SS |
| Self-test FAIL | Module damaged or counterfeit |
| `STATUS_ERROR` on auth | Card held too far from antenna; bring within 3 cm |
| `STATUS_TIMEOUT` on auth | Card removed before auth completed |
| UID keeps changing | Multi-card interference; use one card at a time |
| Manufacturer block all zeros | Counterfeit or blank-UID card; not suitable for deployment |

### Troubleshooting

1. **Firmware version 0x00:** Disconnect 3.3V, wait 2s, reconnect. If still 0x00, swap MISO/MOSI — some modules have silkscreen misprints.
2. **Auth timeout:** MIFARE Classic requires the card to stay stationary within 1–3 cm. The antenna is the rectangular copper coil visible on the PCB.
3. **Module gets hot:** VCC connected to 5V instead of 3.3V — power off immediately, check wiring.
4. **Wrong UID on re-tap:** Some MIFARE cards have anti-collision random UIDs (MIFARE Ultralight C). Use MIFARE Classic 1K for MMS V2.

---

## Test 03 — HC89 Slot Sensor

### Wiring Diagram

```
    ESP32 NodeMCU                    HC89 Module
    ┌──────────────┐                 ┌──────────────┐
    │          3V3 │────────────────►│ VCC (3.3–5V) │
    │          GND │────────────────►│ GND          │
    │      GPIO 32 │◄────────────────│ OUT (D0)     │
    └──────────────┘                 │ A0 (analog,  │
                                     │  not used)   │
                                     └──────────────┘

HC89 output is open-collector with on-board pull-up.
ESP32 INPUT_PULLUP on GPIO 32 provides additional pull-up.

Physical orientation: slot faces the card insertion direction.
The IR beam crosses the slot — card blocks beam → output goes LOW.
```

### Pin Mapping Table

| HC89 Pin | ESP32 GPIO | Notes |
|----------|------------|-------|
| VCC | 3V3 | 3.3V–5V; 3.3V preferred |
| GND | GND | Common ground |
| OUT (D0) | 32 | Digital: LOW = card in slot |
| A0 | — | Analog output — not used |

### Expected Serial Output

```
========================================
 MMS V2 — Test 03: HC89 Slot Sensor
========================================
[INFO] Slot sensor on GPIO 32 (INPUT_PULLUP)
[INFO] Logic: LOW = card present, HIGH = slot empty
[INFO] Debounce: 50 ms
[INFO] Reading sensor...

[STATE] Slot EMPTY  (GPIO=HIGH)    insertions=0  removals=0
[STATE] Slot EMPTY  (GPIO=HIGH)    insertions=0  removals=0
--- Insert card into slot ---
[EVENT] Card INSERTED  debounced=52ms  insertions=1
[STATE] Slot OCCUPIED (GPIO=LOW)   insertions=1  removals=0
[STATE] Slot OCCUPIED (GPIO=LOW)   insertions=1  removals=0
--- Remove card from slot ---
[EVENT] Card REMOVED   debounced=51ms  removals=1
[STATE] Slot EMPTY  (GPIO=HIGH)    insertions=1  removals=1
```

After 10 insert/remove cycles:
```
[SUMMARY] Insertions: 10  Removals: 10  False triggers: 0
[PASS] 10 clean insert/remove cycles detected
```

### Expected OLED Output

None. Serial only.

### Pass/Fail Criteria

- [ ] GPIO 32 reads HIGH when slot is empty
- [ ] GPIO 32 reads LOW when card is in slot
- [ ] State transition is detected within 100 ms of physical insertion
- [ ] Debounce period is 50 ms (no false triggers during bounce window)
- [ ] 10 consecutive insert/remove cycles produce exactly 10 insertions and 10 removals
- [ ] No false triggers counted between cycles

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| Always LOW regardless of card | OUT pin shorted to GND; check wiring |
| Always HIGH even with card | IR LED failed or beam not aligned with card path |
| Bouncy signal (many transitions per insert) | Debounce too short; 50 ms is correct for mechanical slot |
| Signal inverted (LOW = empty) | Using NC variant instead of NO, or inverted module |
| Works for thin cards, fails for thick | Slot width too narrow; check HC89 model (3.7mm vs 6mm slot) |
| Sensor never triggers | Card too thick for slot, or card inserted in wrong orientation |

### Troubleshooting

1. **Always reads HIGH:** Use multimeter on OUT pin — should read ≈0V with card, ≈3.3V without. If stuck at 3.3V always, the IR LED may be burnt out. Look at the LED through a phone camera in the dark — it should glow purple/white with card absent.
2. **Always LOW:** OUT pin may be wired to GND. Disconnect ESP32, check continuity between OUT and GND pins of HC89 module.
3. **Misread on partial card insertion:** The slot needs the card fully inserted to break the beam. Ensure CARD_BLOCK_IDENTITY data block (Block 4) is in the field, which requires full insertion.
4. **Module runs hot:** VCC above 5V. Check power supply.

---

## Test 04 — 128×64 SSD1306 OLED (SPI)

### Wiring Diagram

```
    ESP32 NodeMCU                    SSD1306 SPI OLED (128×64)
    ┌──────────────┐                 ┌─────────────────────┐
    │          3V3 │────────────────►│ VCC                 │
    │          GND │────────────────►│ GND                 │
    │      GPIO 18 │────────────────►│ D0 (SCK/CLK)        │
    │      GPIO 23 │────────────────►│ D1 (MOSI/SDA)       │
    │       GPIO 4 │────────────────►│ RES (RST)           │
    │      GPIO 16 │────────────────►│ CS                  │
    │      GPIO 17 │────────────────►│ DC (A0)             │
    └──────────────┘                 └─────────────────────┘

NOTES:
- This OLED shares the SPI bus with MFRC522.
  Both can be connected simultaneously — CS pins select the active device.
- Some modules label: D0=CLK, D1=MOSI, RES=Reset, DC=data/command
- VCC must be 3.3V. Check module label — some have on-board 3.3V regulator
  and accept 5V input. If VCC pin is labeled "VIN", 5V is safe.
```

### Pin Mapping Table

| OLED Pin | ESP32 GPIO | Function |
|----------|------------|----------|
| VCC | 3V3 | 3.3V power |
| GND | GND | Ground |
| D0 / CLK | 18 | SPI clock |
| D1 / MOSI | 23 | SPI data |
| RES / RST | 4 | Reset (active LOW) |
| CS | 16 | Chip select (active LOW) |
| DC / A0 | 17 | Data/Command select |

### Expected Serial Output

```
========================================
 MMS V2 — Test 04: 128×64 SPI OLED
========================================
[INFO] SSD1306 SPI OLED: 128×64 on DC=17, CS=16, RST=4
[INFO] Initializing display...
[PASS] SSD1306 init OK
[INFO] Running display test sequence (5 pages, 2s each)...
[INFO] Page 1/5 — System info
[INFO] Page 2/5 — Full pixel fill (all ON)
[INFO] Page 3/5 — Horizontal stripes
[INFO] Page 4/5 — MMS V2 IDLE screen simulation
[INFO] Page 5/5 — Progress bar animation
[PASS] All pages displayed
[INFO] Displaying checkerboard pattern for visual inspection...
========================================
 RESULT: PASS — Display responding
 ACTION REQUIRED: Visually confirm display output matches descriptions
========================================
```

### Expected OLED Output

```
Page 1 — System Info:
┌────────────────────────────┐
│ MMS V2 OLED64 Test         │
│ 128x64 SPI SSD1306         │
│ DC:17  CS:16  RST:4        │
│ GPIO18 SCK GPIO23 MOSI     │
│ Test: PASS                 │
│ Adafruit SSD1306           │
│ v2.x.x                     │
└────────────────────────────┘

Page 2 — All Pixels ON:
┌────────────────────────────┐
│████████████████████████████│  (completely white — all 128×64 pixels lit)
│████████████████████████████│
│    ... 64 rows of white ...│
│████████████████████████████│
└────────────────────────────┘

Page 3 — Horizontal Stripes:
┌────────────────────────────┐
│████████████████████████████│
│                            │
│████████████████████████████│
│                            │
│    ... alternating ...     │
└────────────────────────────┘

Page 4 — MMS V2 IDLE Simulation:
┌────────────────────────────┐
│     Tinkerers' Lab         │
│      IIT Indore            │
│                            │
│  [== machine name ==]      │
│                            │
│   Tap card to start        │
└────────────────────────────┘

Page 5 — Progress Bar (animated):
┌────────────────────────────┐
│  Updating FW...            │
│  v2.0.0 → v2.1.0          │
│  [████████████░░░░░░░░]   │
│        62%                 │
│   Do not remove card       │
└────────────────────────────┘
```

### Pass/Fail Criteria

- [ ] `SSD1306 init OK` message (no `[FAIL]` on init)
- [ ] Page 1: text visible and legible at normal viewing distance
- [ ] Page 2: entire display white (no dead rows, no stuck pixels)
- [ ] Page 3: clean alternating stripes (no pixel skipping)
- [ ] Page 4: text layout matches MMS V2 IDLE screen
- [ ] Page 5: progress bar animates smoothly (no tearing)
- [ ] No dark bands across the display (common sign of low VCC)
- [ ] Display does not flicker or reset during test

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| Blank white screen (all on) | DC pin not connected or always HIGH |
| Blank black screen (all off) | Init failed; check SPI wiring |
| Upside-down or mirrored | Module variant; adjust `flipHorizontal` / `flipVertical` in code |
| `SSD1306 allocation failed` | Heap too low; reduce sketch size or use `F()` macro |
| Half display lit | Driver IC fault; replace module |
| Text has wrong characters | MOSI/MISO swapped — data flowing backwards |
| Flickering when MFRC522 active | SPI conflict; ensure CS pins are managed correctly |

### Troubleshooting

1. **Black screen:** Verify 3.3V on VCC with multimeter. Then check RST — hold HIGH during operation. If RST is floating, display resets continuously.
2. **White screen:** DC pin issue. With DC=HIGH the controller expects data bytes, not commands. If DC is floating HIGH during init, display receives garbage and goes all-on.
3. **SPI conflict:** When running this test alongside MFRC522, the OLED CS (GPIO 16) must be held HIGH by default. The Adafruit library handles this automatically, but verify with a scope if needed.
4. **"allocation failed":** The Adafruit driver allocates a 128×64÷8 = 1024 byte framebuffer. If heap is below ~5 KB free, this will fail. Run Test 01 first to verify heap.

---

## Test 05 — 128×32 SSD1306 OLED (I2C)

### Wiring Diagram

```
    ESP32 NodeMCU                    SSD1306 I2C OLED (128×32)
    ┌──────────────┐                 ┌──────────────────────┐
    │          3V3 │────────────────►│ VCC                  │
    │          GND │────────────────►│ GND                  │
    │      GPIO 21 │───────[4.7kΩ]──►│ SDA  (pull-up req'd) │
    │              │          │      └──────────────────────┘
    │              │         3V3
    │      GPIO 22 │───────[4.7kΩ]──►│ SCL  (pull-up req'd) │
    │              │          │      └──────────────────────┘
    └──────────────┘         3V3

NOTES:
- Most SSD1306 I2C modules include 4.7kΩ pull-ups on-board.
  If your module has no pull-ups, add external 4.7kΩ from SDA/SCL to 3.3V.
- Default I2C address: 0x3C (ADDR pin LOW or floating)
  Alternative: 0x3D (ADDR pin HIGH)
- No RST pin used — pass -1 to Adafruit constructor.
- This OLED and the SPI OLED can both be connected simultaneously.
  They use different buses (SPI vs I2C).
```

### Pin Mapping Table

| OLED Pin | ESP32 GPIO | Notes |
|----------|------------|-------|
| VCC | 3V3 | 3.3V power |
| GND | GND | Ground |
| SDA | 21 | I2C data; pull-up to 3.3V |
| SCL | 22 | I2C clock; pull-up to 3.3V |
| ADDR / SA0 | — | Float or tie to GND for 0x3C; tie HIGH for 0x3D |

### Expected Serial Output

```
========================================
 MMS V2 — Test 05: 128×32 I2C OLED
========================================
[INFO] I2C init on SDA=21, SCL=22 at 400kHz
[INFO] Scanning I2C bus...
[INFO]   Address 0x3C — DEVICE FOUND
[INFO]   (No other devices found)
[PASS] SSD1306 found at 0x3C
[INFO] Initializing 128×32 display...
[PASS] SSD1306 init OK
[INFO] Running display test sequence...
[INFO] Page 1/4 — Status bar simulation (MMS V2 layout)
[INFO] Page 2/4 — Full pixel fill
[INFO] Page 3/4 — Vertical stripes
[INFO] Page 4/4 — Scrolling text
[PASS] All pages displayed
========================================
 RESULT: PASS
 ACTION REQUIRED: Visually confirm display output
========================================
```

If no device found:
```
[INFO] Scanning I2C bus...
[INFO]   No devices found on bus
[FAIL] No SSD1306 at 0x3C or 0x3D — check SDA/SCL wiring
```

### Expected OLED Output

```
Page 1 — MMS V2 Status Strip simulation:
┌────────────────────────────────┐  (128 pixels wide)
│ ● ●   SOLDERING STA 1   14:58 │
│                                │
│ WiFi/MQTT icons (left), name   │
│ (center), time (right)         │
└────────────────────────────────┘  (32 pixels tall)

Page 2 — All pixels ON:
┌────────────────────────────────┐
│████████████████████████████████│
│████████████████████████████████│
│████████████████████████████████│
│████████████████████████████████│
└────────────────────────────────┘

Page 3 — Vertical stripes:
┌────────────────────────────────┐
│█ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █│
│█ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █│
│█ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █│
│█ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █│
└────────────────────────────────┘
```

### Pass/Fail Criteria

- [ ] I2C scan finds device at `0x3C` (or `0x3D`)
- [ ] `SSD1306 init OK` message
- [ ] All 4 pages display without crash
- [ ] Status bar layout is legible on 32-pixel height
- [ ] Text does not overflow outside the 32-pixel boundary
- [ ] No pixel noise when SPI OLED (Test 04) is also connected

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| Nothing found on I2C scan | Missing pull-ups; or SDA/SCL swapped |
| Found at 0x3D not 0x3C | ADDR pin tied HIGH; update address in Test 05 code |
| Init OK but display blank | 32-pixel height declared but 64-pixel module connected |
| Garbled display | I2C bus speed too high; reduce to 100 kHz |
| Both I2C devices conflict | Only one SSD1306 should be on each I2C address |

### Troubleshooting

1. **I2C scan empty:** Check pull-ups. With no pull-ups, the bus stays LOW and the controller sees all zeros. Use a multimeter: SDA and SCL should read 3.3V when idle.
2. **Wrong address:** Add `Wire.begin(21,22); Wire.beginTransmission(0x3D); if(Wire.endTransmission()==0) Serial.println("Found at 0x3D");`
3. **Display shows 64 pixels of content on 32 display:** Height parameter in constructor must be 32, not 64.

---

## Test 06 — Mechanical Relay

### Wiring Diagram

```
    ESP32 NodeMCU           Relay Module (5V Coil)
    ┌──────────────┐        ┌─────────────────────────┐
    │         5V  │───────►│ VCC (coil power — 5V)   │
    │          GND │───────►│ GND                     │
    │      GPIO 26 │───────►│ IN  (control signal)    │
    └──────────────┘        └────────────┬────────────┘
                                         │
                              ┌──────────▼──────────┐
                              │  RELAY CONTACTS     │
                              │   COM ──── NO       │
                              │       └── NC        │
                              └─────────────────────┘

SAFETY WARNING:
  This test validates only the relay switching action.
  DO NOT connect mains voltage during this test.
  Use a multimeter in continuity mode on the relay contacts.

Active HIGH module (most common with optocoupler):
  GPIO 26 = HIGH → relay energised → NO closes, NC opens
  GPIO 26 = LOW  → relay de-energised → NO opens, NC closes

Active LOW module (some blue modules without optocoupler):
  GPIO 26 = LOW  → relay energised
  GPIO 26 = HIGH → relay de-energised

The V2 firmware reads relay_active_high from NVS at boot.
Test both modes using the serial command interface below.
```

### Pin Mapping Table

| Relay Terminal | Connection | Notes |
|----------------|------------|-------|
| VCC / DC+ | ESP32 5V | Coil needs 5V; 3.3V may not energise reliably |
| GND / DC- | ESP32 GND | Common ground |
| IN | GPIO 26 | Control input |
| COM | Multimeter probe A | Common contact |
| NO | Multimeter probe B | Normally Open |
| NC | — | Normally Closed (not used in MMS V2 SSR path) |

### Expected Serial Output

```
========================================
 MMS V2 — Test 06: Mechanical Relay
========================================
[INFO] Relay control on GPIO 26
[INFO] Starting in SAFE STATE (relay OFF)
[INFO] Send commands via Serial Monitor (115200 baud):
[INFO]   'h' = active HIGH mode (IN HIGH = relay ON)
[INFO]   'l' = active LOW mode  (IN LOW  = relay ON)
[INFO]   'o' = relay ON
[INFO]   'f' = relay OFF
[INFO]   't' = timed pulse (500ms ON, then OFF)
[INFO]   'c' = 10-cycle test (100ms ON / 100ms OFF)
[INFO] Current mode: ACTIVE HIGH
[INFO] Relay state: OFF (GPIO 26 = LOW)

--- send 'o' ---
[ACTION] Relay ON  — GPIO 26 = HIGH
[AUDIO]  Click heard? ← confirm manually

--- send 'f' ---
[ACTION] Relay OFF — GPIO 26 = LOW
[AUDIO]  Click heard? ← confirm manually

--- send 'c' ---
[INFO] Starting 10-cycle test...
[INFO] Cycle 1/10 — ON
[INFO] Cycle 1/10 — OFF
...
[INFO] Cycle 10/10 — OFF
[PASS] 10 cycles complete — no errors
[INFO] Final state: RELAY OFF (safe)
```

### Expected OLED Output

None. Serial monitor interface only.

### Pass/Fail Criteria

- [ ] Relay clicks audibly when ON command sent (coil energising)
- [ ] Relay clicks audibly when OFF command sent (coil de-energising)
- [ ] Multimeter reads continuity on COM→NO when relay is ON
- [ ] Multimeter reads open on COM→NO when relay is OFF
- [ ] GPIO 26 LOW on boot (safe state before any command)
- [ ] 10-cycle test completes with no stuck states
- [ ] No relay stays permanently energised after OFF command

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| No click, relay stays off | VCC not 5V; or IN signal too low |
| Relay stuck ON | Active LOW module in active HIGH mode; swap mode |
| Click but contacts don't switch | Relay welded or failed internally |
| ESP32 resets when relay activates | Coil back-EMF; relay module needs flyback diode |
| Relay chatters | VCC drooping under coil load; add 100µF cap to VCC |

### Troubleshooting

1. **No click:** Measure VCC pin — must be 4.8–5.2V under load. Measure IN pin with GPIO HIGH — must be 3.0–3.3V. If relay still won't trigger, it requires 5V logic level on IN; add a 2N2222 transistor driver.
2. **ESP32 resets:** The relay module's built-in flyback diode may be missing or reverse-installed. Add a 1N4007 across the coil terminals (cathode toward VCC). Also ensure ESP32 5V and the relay 5V share the same ground.
3. **Contacts not switching:** Measure COM→NO with multimeter in ohm mode. <1Ω = closed. If always open, the mechanical arm may be jammed. Listen for the electromagnetic click — if you hear click but contacts don't switch, the relay is internally broken.

---

## Test 07 — Solid State Relay (SSR)

### Wiring Diagram

```
    ESP32 NodeMCU           SSR Module (e.g. Fotek SSR-40 DA)
    ┌──────────────┐        ┌─────────────────────────────┐
    │          3V3 │───┐    │   Control side              │
    │      GPIO 26 │──►│    │   (+) INPUT+ (3–32V DC)     │
    │              │   └───►│   (+) INPUT+                │
    │          GND │───────►│   (-) INPUT-                │
    └──────────────┘        │                             │
                            │   Load side (AC/DC)         │
                            │   L1 / LOAD1                │
                            │   L2 / LOAD2                │
                            └─────────────────────────────┘

NOTES:
- SSR control input accepts 3–32V DC. ESP32 3.3V GPIO drives it directly.
  No series resistor needed on the control input.
- Active HIGH: GPIO 26 HIGH (3.3V) → SSR conducts → load energised.
- SSR turns on at zero-cross (for AC SSR). DC SSR turns on immediately.
- DO NOT connect mains voltage during this test. Test contact state only.
- Some SSRs have a built-in status LED on the control side (green = armed).
```

### Pin Mapping Table

| SSR Terminal | Connection | Notes |
|--------------|------------|-------|
| INPUT+ (3) | GPIO 26 | 3.3V GPIO drives directly |
| INPUT- (4) | GND | Common ground |
| LOAD1 (1) | Multimeter probe A | Load terminal |
| LOAD2 (2) | Multimeter probe B | Load terminal |

### Expected Serial Output

```
========================================
 MMS V2 — Test 07: Solid State Relay
========================================
[INFO] SSR control on GPIO 26
[INFO] SSR type: Active HIGH (3.3V GPIO = ON)
[INFO] Initial state: SSR OFF (GPIO 26 = LOW)
[INFO] Commands: 'o'=ON, 'f'=OFF, 'p'=pulse 500ms, 'c'=10-cycle test

--- send 'o' ---
[ACTION] SSR ON  — GPIO 26 = 3.3V
[INFO]   Status LED on SSR should illuminate now
[INFO]   Measure LOAD1↔LOAD2: expect LOW RESISTANCE (SSR conducting)

--- send 'f' ---
[ACTION] SSR OFF — GPIO 26 = 0V
[INFO]   Status LED should extinguish
[INFO]   Measure LOAD1↔LOAD2: expect OPEN CIRCUIT

--- send 'c' ---
[INFO] 10-cycle test: 500ms ON / 500ms OFF
[INFO] Cycle 1/10 ON... OFF
...
[PASS] 10-cycle test complete
[INFO] SSR safely OFF
```

### Pass/Fail Criteria

- [ ] SSR status LED illuminates when GPIO 26 HIGH
- [ ] SSR status LED off when GPIO 26 LOW
- [ ] LOAD terminals measure <1Ω (continuity) when control ON
- [ ] LOAD terminals measure open circuit when control OFF
- [ ] 10-cycle test completes with no stuck state
- [ ] No residual voltage on LOAD terminals when OFF (check with multimeter DC mode)
- [ ] GPIO 26 starts LOW (safe state on boot)

### Differences from Mechanical Relay

| Property | Mechanical | Solid State |
|----------|-----------|-------------|
| Audible click | Yes | No |
| Switching speed | ~10ms | <1ms |
| Minimum load | None | Some SSRs need 100mA min load |
| Zero-cross detection | No | Yes (AC SSR) |
| Active HIGH threshold | 5V typical | 3–32V (3.3V works) |
| Back-EMF risk | Yes (inductive load) | No |

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| SSR doesn't turn on at 3.3V | SSR requires 5V minimum; check datasheet |
| LOAD leaks voltage when OFF | Normal for SSR — some have 1–3V off-state leakage |
| SSR stays ON (welded) | Excessive load current exceeded SSR rating |
| Control LED on but load not switching | LED in parallel with coil — verify load wiring |

---

## Test 08 — WS2812 RGB LED

### Wiring Diagram

```
    ESP32 NodeMCU              WS2812 / NeoPixel Module
    ┌──────────────┐           ┌─────────────────────┐
    │           5V │──────────►│ VCC (5V)            │
    │          GND │──────────►│ GND                 │
    │      GPIO 33 │──[470Ω]──►│ DIN (Data In)       │
    └──────────────┘           │ DOUT (to next LED)  │
                               └─────────────────────┘
         [100µF electrolytic capacitor across VCC–GND at module]
         Capacitor positive lead → VCC. This prevents power-on voltage spike.

NOTES:
- WS2812 requires 5V power. The ESP32 5V pin (Vin/5V) passes USB 5V through.
  This only works when powered via USB (not battery). Max current: ~500mA USB.
- WS2812B works at 3.3V data from ESP32 GPIO directly.
  WS2812 (original, non-B) may require a 5V data level — add a level shifter
  (74HCT245 or similar) if data signal unreliable.
- 470Ω series resistor on data line protects against ringing / ESD.
- Each LED draws 60mA at full white. A single LED is fine. Strings of 10+
  need a separate 5V supply, not USB.
```

### Pin Mapping Table

| WS2812 Pin | ESP32 Connection | Notes |
|------------|-----------------|-------|
| VCC | 5V (Vin) | USB-sourced 5V only |
| GND | GND | Common ground |
| DIN | GPIO 33 via 470Ω | Data input |
| DOUT | — | Next LED in chain (unused for 1 LED) |

### Expected Serial Output

```
========================================
 MMS V2 — Test 08: WS2812 RGB LED
========================================
[INFO] WS2812 on GPIO 33, 1 LED, 470Ω series resistor
[INFO] Starting color test sequence (2s per color)...

[INFO] RED   (255, 0, 0)
[INFO] GREEN (0, 255, 0)
[INFO] BLUE  (0, 0, 255)
[INFO] WHITE (255, 255, 255)
[INFO] OFF   (0, 0, 0)
[PASS] Basic color test complete

[INFO] Starting color wheel sweep (256 steps)...
[PASS] Color wheel complete

[INFO] Starting brightness sweep (0→255→0)...
[PASS] Brightness sweep complete

[INFO] MMS V2 state color simulation:
[INFO]   IDLE          — dim white, breathing
[INFO]   SESSION       — solid green
[INFO]   WARNING       — alternating yellow/orange
[INFO]   ACCESS DENIED — 3× red flash, then off
[PASS] State simulation complete
========================================
 RESULT: PASS
 ACTION REQUIRED: Visually confirm LED colors match descriptions
========================================
```

### Expected OLED Output

None. Visual inspection of LED only.

### Pass/Fail Criteria

- [ ] Red: only red channel lit, no green/blue bleed
- [ ] Green: only green channel lit
- [ ] Blue: only blue channel lit
- [ ] White: all three channels at equal brightness; LED appears white
- [ ] Off: LED completely dark
- [ ] Color wheel cycles through full hue spectrum without gaps
- [ ] Breathing animation is smooth (no abrupt jumps)
- [ ] No flicker between states

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| LED always off | 5V power not reaching module; or data line fault |
| LED solid wrong color | Library pixel order set to GRB but module is RGB (or vice versa) |
| LED flickering | Insufficient power; or no decoupling capacitor |
| LED works once then stops | Power-on spike destroyed driver IC; add 100µF cap |
| Colors correct but dim | Using 3.3V power instead of 5V |
| String stops mid-way | Later LEDs not receiving data; check DOUT→DIN connections |

### Troubleshooting

1. **No response:** Measure 5V at VCC. If present, check data line — with a multimeter in AC mode, briefly touching the data line while the sketch runs should show slight deflection (data pulses).
2. **Wrong colors:** Adafruit NeoPixel default order is GRB. If red command shows green, add `NEO_RGB` to the constructor: `Adafruit_NeoPixel(1, 33, NEO_RGB + NEO_KHZ800)`.
3. **Only first LED works in a string:** Each LED forwards data to DOUT→DIN of next LED. Check DOUT connection and that all GNDs are connected.
4. **Dimmer than expected:** WS2812 at 5V with 3.3V data — add a 74HCT125 or 74HCT245 level shifter to bring data to 5V.

---

## Test 09 — Rotary Encoder (EC11)

### Wiring Diagram

```
    ESP32 NodeMCU              EC11 Rotary Encoder
    ┌──────────────┐           ┌────────────────────┐
    │          3V3 │──────────►│ VCC (+)             │
    │          GND │──────────►│ GND (-)             │
    │      GPIO 34 │◄──────────│ CLK (A)             │
    │      GPIO 35 │◄──────────│ DT  (B)             │
    │      GPIO 25 │◄──────────│ SW  (push button)   │
    └──────────────┘           └────────────────────┘

NOTES:
- GPIO 34 and 35 are INPUT ONLY — no internal pull-ups available.
  Add 10kΩ external pull-up resistors from CLK and DT to 3.3V.
- GPIO 25 has internal pull-up (INPUT_PULLUP used in firmware).
  External pull-up optional but recommended for clean signal.
- Most EC11 breakout boards include pull-ups on-board.
  Check module schematic before adding external resistors.
- The encoder is Phase 4 hardware — not populated on initial deployment.
  This test validates the wiring before software integration.

External pull-up connections (if module has no on-board pull-ups):
    3V3 ──[10kΩ]── GPIO 34 (CLK)
    3V3 ──[10kΩ]── GPIO 35 (DT)
```

### Pin Mapping Table

| EC11 Pin | ESP32 GPIO | Pull-up | Notes |
|----------|------------|---------|-------|
| VCC (+) | 3V3 | — | Power |
| GND (-) | GND | — | Ground |
| CLK (A) | 34 | External 10kΩ | Input-only pin |
| DT (B) | 35 | External 10kΩ | Input-only pin |
| SW | 25 | Internal (INPUT_PULLUP) | Push button |

### Expected Serial Output

```
========================================
 MMS V2 — Test 09: Rotary Encoder
========================================
[INFO] Encoder on CLK=34, DT=35, SW=25
[INFO] CLK/DT: INPUT (no internal pull-up — external 10kΩ required)
[INFO] SW: INPUT_PULLUP
[INFO] Rotate CW to increment, CCW to decrement, press to reset

[STATE] Position: 0    Button: RELEASED
[STATE] Position: 0    Button: RELEASED
--- rotate CW ---
[EVENT] CW  step — Position: 1
[EVENT] CW  step — Position: 2
[EVENT] CW  step — Position: 3
--- rotate CCW ---
[EVENT] CCW step — Position: 2
[EVENT] CCW step — Position: 1
--- press button ---
[EVENT] BUTTON PRESSED
[EVENT] BUTTON RELEASED  (held for 48ms)
[EVENT] Position RESET to 0
[STATE] Position: 0    Button: RELEASED

[PASS] CW rotation detected
[PASS] CCW rotation detected
[PASS] Button press detected
[PASS] Button release detected
========================================
 RESULT: PASS — all encoder events functional
========================================
```

### Pass/Fail Criteria

- [ ] CLK pin reads HIGH when encoder is stationary
- [ ] DT pin reads HIGH when encoder is stationary
- [ ] CW rotation increments position counter by 1 per detent
- [ ] CCW rotation decrements position counter by 1 per detent
- [ ] No spurious counts between detents (bouncy CLK)
- [ ] SW reads HIGH when button released
- [ ] SW reads LOW when button pressed
- [ ] Button hold duration reported correctly (within ±10ms)

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| Random counting without rotation | CLK/DT floating; add external pull-ups |
| CW and CCW both increment | CLK and DT wired to same GPIO; check wiring |
| Direction reversed | Swap CLK and DT signals |
| Misses steps | GPIO interrupt latency; ensure ISR priority is adequate |
| Button always reads 0 | SW connected to GND-only pin without pull-up |
| Count jumps by 2 per detent | EC11 has 2 pulses per detent; divide by 2 in logic |

### Troubleshooting

1. **Random counting:** GPIO 34/35 have no internal pull-ups. Without external 10kΩ pull-ups, the pins float and trigger randomly. Confirm pull-ups with multimeter: CLK and DT should read 3.3V at rest.
2. **Direction wrong:** Swap CLK (GPIO 34) and DT (GPIO 35) in wiring only — do not change code.
3. **Misses detents:** The ISR fires on CLK rising edge. If the encoder bounces, software debounce of 2–5ms on CLK is needed.
4. **EC11 doesn't respond at all:** Some EC11 modules have an internal pull-up to VCC. If VCC is not connected, CLK/DT will float. Connect VCC to 3.3V.

---

## Test 10 — WiFi

### Wiring Diagram

None required. Internal WiFi antenna.

### Configuration

Edit `10_wifi.ino` before flashing:
```cpp
#define WIFI_SSID     "your_network_name"
#define WIFI_PASSWORD "your_network_password"
```

### Expected Serial Output

```
========================================
 MMS V2 — Test 10: WiFi
========================================
[INFO] Phase 1: Network scan
[INFO] Scanning...
[INFO] Found 5 networks:
[INFO]   -42 dBm  [WPA2]  tlmms
[INFO]   -67 dBm  [WPA2]  AndroidAP
[INFO]   -71 dBm  [WPA2]  LabNet
[INFO]   -75 dBm  [WPA2]  HomeRouter
[INFO]   -89 dBm  [WPA ]  Old_Network
[PASS] Target network "tlmms" found at -42 dBm (EXCELLENT)

[INFO] Phase 2: Connection
[INFO] Connecting to "tlmms"...
[INFO] . . . . .
[PASS] Connected in 3240ms
[PASS] IP address   : 192.168.0.47
[PASS] Subnet mask  : 255.255.255.0
[PASS] Gateway      : 192.168.0.1
[PASS] DNS          : 192.168.0.1
[INFO] BSSID (AP MAC): AA:BB:CC:DD:EE:FF
[INFO] Channel      : 6
[INFO] RSSI         : -42 dBm (EXCELLENT)

[INFO] Phase 3: NTP time sync
[INFO] NTP server: pool.ntp.org (IST = UTC+5:30)
[INFO] Syncing... . . .
[PASS] NTP sync OK — 2025-06-15 14:58:23 IST

[INFO] Phase 4: Reconnect test
[INFO] Disconnecting...
[INFO] Reconnecting...
[PASS] Reconnect OK in 1820ms
[INFO] RSSI after reconnect: -43 dBm

========================================
 RESULT: 4/4 phases passed
 STATUS: PASS
========================================

[LOOP] Uptime: 30s  RSSI: -42dBm  Heap: 287332 bytes
[LOOP] Uptime: 60s  RSSI: -41dBm  Heap: 287332 bytes
```

RSSI interpretation:
- > -60 dBm: EXCELLENT — good deployment location
- -60 to -70 dBm: GOOD — acceptable
- -70 to -80 dBm: FAIR — consider relocating node closer to AP
- < -80 dBm: POOR — unreliable; node must be moved

### Pass/Fail Criteria

- [ ] Target SSID visible in scan
- [ ] RSSI of target network > -80 dBm at installation location
- [ ] Connection established within 10 seconds
- [ ] Valid IP address assigned (not 0.0.0.0)
- [ ] NTP sync succeeds (time is current, not 1970-01-01)
- [ ] Reconnect after forced disconnect succeeds within 5 seconds
- [ ] RSSI does not vary by more than 10 dBm between measurements (stable placement)

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| SSID not found in scan | Wrong SSID, or AP on 5 GHz (ESP32 is 2.4 GHz only) |
| `WiFi.begin` never connects | Wrong password; or BSSID filter on AP |
| IP 0.0.0.0 | DHCP not responding; assign static IP if needed |
| NTP fails | No internet access; or port 123 blocked |
| RSSI < -80 | Node too far from AP; add WiFi extender or move AP |
| Frequent disconnects | AP channel congestion; change AP channel |

### Troubleshooting

1. **SSID not in scan:** ESP32 WiFi is 2.4 GHz only. If lab AP is 5 GHz-only, add a 2.4 GHz band or use a separate AP.
2. **Connection timeout:** Verify password has no leading/trailing spaces. Special characters in SSID or password must be escaped in `#define`.
3. **NTP wrong time:** `configTime(19800, 0, ...)` sets UTC+5:30 (IST). Offset = 19800 seconds. If wrong timezone, adjust offset.
4. **Heap drops over time:** Memory leak in sketch. Not applicable to V2 firmware which manages its own heap carefully.

---

## Test 11 — MQTT

### Wiring Diagram

None required. Uses WiFi (Test 10 must pass first).

### Configuration

Edit `11_mqtt.ino` before flashing:
```cpp
#define WIFI_SSID     "your_ssid"
#define WIFI_PASSWORD "your_password"
#define MQTT_BROKER   "192.168.0.10"   // Raspberry Pi static IP
#define MQTT_PORT     1883
#define MQTT_USER     "bridge"          // from generate_mqtt_creds.py
#define MQTT_PASSWORD "your_bridge_password"
```

### Expected Serial Output

```
========================================
 MMS V2 — Test 11: MQTT
========================================
[INFO] WiFi: connecting to "tlmms"...
[PASS] WiFi connected — IP: 192.168.0.47

[INFO] Phase 1: Broker connection
[INFO] Connecting to MQTT broker 192.168.0.10:1883...
[PASS] MQTT connected (client ID: mms_test_mqtt)

[INFO] Phase 2: Subscribe to test topic
[INFO] Subscribing to mms/test/response (QoS 1)...
[PASS] Subscribe OK

[INFO] Phase 3: Publish and loopback test
[INFO] Publishing to mms/test/echo: {"id":1,"msg":"hello","ts":1718600000}
[PASS] Publish acknowledged (QoS 1)
[INFO] Waiting for broker to echo back on mms/test/response...
[RECV] mms/test/response: {"id":1,"msg":"hello","ts":1718600000}
[PASS] Loopback received in 23ms

[INFO] Phase 4: QoS 0 vs QoS 1 latency
[INFO] QoS 0 average latency: 8ms  (5 messages)
[INFO] QoS 1 average latency: 24ms (5 messages, includes PUBACK)
[PASS] QoS 1 latency < 500ms

[INFO] Phase 5: Retained message test
[INFO] Publishing retained: mms/test/retain = "test_value"
[PASS] Published retained
[INFO] Unsubscribing...
[INFO] Resubscribing to mms/test/retain...
[RECV] mms/test/retain: "test_value"  (retained flag set)
[PASS] Retained message received on resubscribe

[INFO] Phase 6: Disconnect and reconnect
[INFO] Disconnecting...
[INFO] Reconnecting...
[PASS] Reconnected in 312ms

[INFO] Cleaning up test topics...
[INFO] Published empty retained to mms/test/retain (clearing)
========================================
 RESULT: 6/6 phases passed
 STATUS: PASS
========================================
```

If broker authentication fails:
```
[FAIL] MQTT connect refused — rc=5 (bad credentials)
       Check MQTT_USER and MQTT_PASSWORD in test sketch
       Verify user exists in /etc/mosquitto/passwd
```

MQTT return codes: 0=accepted, 1=bad protocol, 2=client ID rejected, 3=broker unavailable, 4=bad user/pass, 5=unauthorised.

### Pass/Fail Criteria

- [ ] Broker connection accepted (rc=0)
- [ ] Subscribe returns success
- [ ] Published QoS 1 message receives PUBACK
- [ ] Loopback message received within 500ms
- [ ] Retained message received on re-subscribe after disconnect
- [ ] Reconnect after disconnect succeeds within 5 seconds
- [ ] No duplicate messages received

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| rc=3 (unavailable) | Mosquitto not running on Pi; `systemctl start mosquitto` |
| rc=4 (bad user/pass) | Wrong credentials; check generate_mqtt_creds.py output |
| rc=5 (unauthorised) | ACL blocks this client; check `/etc/mosquitto/acl` |
| No loopback received | Bridge not running or topic routing issue |
| PUBACK never received | Broker QoS 1 ACK timeout; check broker logs |
| Connection drops every 60s | Keepalive mismatch; set `setKeepAlive(60)` in PubSubClient |

### Troubleshooting

1. **Broker unreachable:** `ping 192.168.0.10` from laptop. If Pi is reachable but MQTT isn't: `sudo netstat -tlnp | grep 1883` on Pi.
2. **Authentication failure:** `mosquitto_sub -h 192.168.0.10 -u bridge -P password -t '#'` — if this works, credentials are correct and the issue is in the test sketch.
3. **No loopback:** The loopback test requires Mosquitto to route `mms/test/echo` to `mms/test/response`. If bridge.py is not configured for this, use a direct subscribe to `mms/test/echo` instead — check the sketch's `ECHO_TOPIC` define.

---

## Test 12 — LittleFS

### Wiring Diagram

None required. Internal flash storage.

### Expected Serial Output

```
========================================
 MMS V2 — Test 12: LittleFS
========================================
[INFO] Phase 1: Mount
[INFO] Mounting LittleFS...
[PASS] Mounted OK
[INFO] Total bytes: 1458176  (1.39 MB)
[INFO] Used bytes:  0         (fresh partition)

[INFO] Phase 2: Write and read back
[INFO] Writing /test/hello.txt...
[PASS] Write OK (28 bytes)
[INFO] Reading /test/hello.txt...
[PASS] Read OK — content matches exactly
[PASS] File size correct: 28 bytes

[INFO] Phase 3: Append
[INFO] Appending 5 lines to /logs/sessions.jsonl...
[PASS] Append OK — 5 lines written
[INFO] Reading back all lines...
[PASS] Line 1 valid JSON: {"t":1718600000,"r":"220002123","m":1,"e":"session_end","d":245,"er":"card_removed"}
[PASS] Line 2 valid JSON
[PASS] Line 3 valid JSON
[PASS] Line 4 valid JSON
[PASS] Line 5 valid JSON
[PASS] All 5 lines read correctly

[INFO] Phase 4: Rotation test (simulating LOG_ROTATE_BYTES = 51200)
[INFO] Writing log lines until 50 KB...
[INFO] ... 100 lines (6200 bytes)
[INFO] ... 200 lines (12400 bytes)
[INFO] ... 400 lines (24800 bytes)
[INFO] ... 800 lines (49600 bytes)
[INFO] Rotation threshold reached at 806 lines (50013 bytes)
[PASS] /logs/sessions.jsonl renamed to /logs/sessions.old.jsonl
[PASS] New /logs/sessions.jsonl created (empty)
[INFO] Old log preserved: 50013 bytes

[INFO] Phase 5: Directory listing
[PASS] /logs/sessions.jsonl  — 0 bytes
[PASS] /logs/sessions.old.jsonl — 50013 bytes
[PASS] /test/hello.txt — 28 bytes

[INFO] Phase 6: Delete and free space
[INFO] Removing test files...
[PASS] /test/hello.txt deleted
[PASS] /logs/sessions.jsonl deleted
[PASS] /logs/sessions.old.jsonl deleted
[INFO] Used bytes after cleanup: 0
[PASS] Free space fully recovered

========================================
 RESULT: 6/6 phases passed
 STATUS: PASS
========================================
```

### Pass/Fail Criteria

- [ ] LittleFS mounts without format error
- [ ] Total partition size ≥ 1 MB
- [ ] Write 28 bytes → read back identical bytes
- [ ] Append 5 lines → each line reads back as valid JSON
- [ ] Log rotation triggers at configured byte threshold
- [ ] Old log file preserved after rotation
- [ ] Directory listing returns correct file names and sizes
- [ ] File deletion frees space (used bytes returns to pre-write value)
- [ ] Two consecutive mounts (simulate reboot) both succeed

### Common Failure Modes

| Symptom | Cause |
|---------|-------|
| Mount fails, auto-formats | Corrupt filesystem; normal on first use, then PASS |
| Total size 0 or very small | Wrong partition scheme; re-select in Arduino IDE |
| Write fails (no space) | Partition too small; or previous test didn't clean up |
| Line read back garbled | Encoding issue; ensure all writes use UTF-8 |
| Rotation renames but old file missing | File system inconsistency; add fsync equivalent |
| Heap error during large write | Reduce buffer size in test sketch |

### Troubleshooting

1. **Partition too small:** Arduino IDE → Tools → Partition Scheme → "Default 4MB with spiffs" gives 1.5 MB for SPIFFS/LittleFS. Verify with `LittleFS.totalBytes()`.
2. **Mount fails repeatedly:** Full erase via Arduino IDE → Erase All Flash → upload empty sketch → then re-upload test.
3. **File content garbled:** Use binary-safe write functions. Avoid `println()` for JSON lines — use `print(line + "\n")` to control line endings exactly.
4. **Rotation not triggering:** Check `LOG_ROTATE_BYTES` define in the test sketch matches `config.h` value (51200 = 50 KB).
