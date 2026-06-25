# MMS V2 — Hardware Validation Execution Plan

**Audience:** Junior engineer performing first-time component validation
**Goal:** Verify every hardware component independently before running integrated firmware
**Time:** ~2.5–3 hours for all 12 tests end-to-end

---

## Before You Start

### Equipment checklist

| Item | Qty | Notes |
|---|---|---|
| ESP32 NodeMCU 38-pin | 1 | Any 38-pin variant |
| USB-A to Micro-USB cable | 1 | Must carry data, not charge-only |
| Laptop with Arduino IDE 2.x | 1 | Or VS Code + PlatformIO |
| Breadboard (830 tie-point) | 1 | |
| Jumper wires M-M | 20+ | |
| Jumper wires M-F | 10+ | For module connectors |
| Multimeter | 1 | Continuity + voltage modes |
| MFRC522 RFID module | 1 | With header pins soldered |
| MIFARE Classic 1K card | 1 | Blank or pre-used |
| HC89 slot sensor | 1 | Optical IR slot type |
| SSD1306 128×64 SPI OLED | 1 | 7-pin SPI version |
| SSD1306 128×32 I2C OLED | 1 | 4-pin I2C version |
| Mechanical relay module | 1 | Single channel, 5V coil |
| Solid State Relay (SSR) | 1 | e.g. Fotek SSR-25DA or equivalent |
| WS2812B LED (1-pixel or strip) | 1 | Pre-wired or strip cut to 1 LED |
| Rotary encoder EC11 | 1 | With push-button, breakout preferred |
| 470Ω resistor | 1 | For WS2812 data line |
| 10kΩ resistor | 2 | For encoder CLK/DT pull-ups |
| 100µF electrolytic capacitor | 1 | For WS2812 VCC decoupling |
| WiFi access point (2.4 GHz) | 1 | SSID must be "tlmms" or update define |
| Raspberry Pi with Mosquitto | 1 | Required for Test 11 only |
| Current sensor module (ACS712 or similar) | 1 | Required for Test 13 only |

### Arduino IDE setup (one-time)

1. **Add ESP32 board package**
   File → Preferences → Additional Boards Manager URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   Tools → Board → Boards Manager → search "esp32" → install **esp32 by Espressif Systems** (3.x)

2. **Board settings** (apply to every sketch):
   - Board: `ESP32 Dev Module`
   - Upload Speed: `921600`
   - Flash Mode: `QIO`
   - Flash Size: `4MB (32Mb)`
   - Partition Scheme: `Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS)` — required for LittleFS Test 12
   - Port: whichever COM/ttyUSB appears when ESP32 is plugged in

3. **Install libraries** (Tools → Manage Libraries):
   | Library | Version | Tests |
   |---|---|---|
   | Adafruit SSD1306 | 2.5.x | 04, 05 |
   | Adafruit GFX Library | 1.11.x | 04, 05 |
   | MFRC522 by miguelbalboa | 1.4.x | 02 |
   | Adafruit NeoPixel | 1.12.x | 08 |
   | PubSubClient by Nick O'Leary | 2.8.x | 11 |
   | ArduinoJson | 7.x | 12 |

   Adafruit BusIO (dependency) installs automatically with SSD1306.

4. **Serial Monitor settings**: 115200 baud, line ending = None (or Newline — doesn't matter)

---

## Recommended Execution Order

Run in this sequence. Each phase uses the same wiring group, so you minimise re-wiring.

| Phase | Tests | Wiring group | Estimated time |
|---|---|---|---|
| A | 01, 12, 10 | No external wiring | 20 min |
| B | 11 | No external wiring (needs RPi) | 15 min |
| C | 05 | I2C (2 wires) | 15 min |
| D | 04, 02 | SPI (4 shared wires + extras) | 25 min |
| E | 03, 06, 07, 08 | Individual GPIO | 40 min |
| F | 09 | GPIO + external pull-ups | 20 min |
| G | 13 | 1 analog wire (GPIO 36) | 10 min |

**Total: ~2h 25m** plus time to flash each sketch (~30–60s each).

---

---

# Phase A — No External Wiring

---

## Test 01 — ESP32 System

**Hardware required:** ESP32, USB cable
**Hardware optional:** None
**Estimated time:** 10 minutes

### Wiring

```
[USB-A] ─────────────────── [ESP32 Micro-USB]
```
No breadboard wiring. USB provides power and serial.

### Required libraries
None beyond ESP32 Arduino core.

### Arduino IDE / PlatformIO setup
- Open `01_esp32_system/01_esp32_system.ino`
- Verify board = `ESP32 Dev Module`
- Upload → open Serial Monitor at 115200

### Expected serial output
```
========================================
 MMS V2 — Test 01: ESP32 System
========================================
[INFO] === Phase 1: Chip Identification ===
[INFO] Chip model  : ESP32-D0WD
[INFO] Chip cores  : 2
[INFO] Revision    : 3
[INFO] Features    : WiFi / BT / BLE
[INFO] MAC address : AA:BB:CC:DD:EE:FF
[PASS] Dual-core ESP32 detected
[INFO] CPU freq: 240 MHz
[PASS] CPU frequency >= 80 MHz

[INFO] === Phase 2: Flash Memory ===
[INFO] Flash size  : 4194304 bytes (4096 KB)
[PASS] Flash size >= 4MB

[INFO] === Phase 3: RAM / Heap ===
[INFO] Free heap   : 284312 bytes
[INFO] Min heap    : 284312 bytes
[PASS] Free heap >= 100KB

[INFO] === Phase 4: NVS Round-trip ===
[INFO] Writing key "hw_test" = 42 to namespace "mms_config"...
[INFO] Reading back key "hw_test"...
[INFO] Read value  : 42
[PASS] NVS write/read round-trip successful

[INFO] === Phase 5: LittleFS Mount + Write/Read ===
[PASS] LittleFS mounted
[PASS] File write/read round-trip verified
[INFO] LittleFS free: 1234 KB / 1520 KB total

[INFO] === Phase 6: GPIO Blink (LED on GPIO 2) ===
[INFO] Blinking onboard LED 5 times...
[INFO] Blink 1
...
[INFO] Blink 5
[PASS] GPIO blink complete

========================================
 RESULT: 8/8 passed
 STATUS: PASS
========================================
[INFO] Heap monitor: every 5s. Press reset to re-run.
[LOOP] Uptime=5s  heap=284312B  min_heap=280000B
```

### Expected display output
None — this sketch uses no display.

### Pass criteria
- All 8 checks print `[PASS]`
- No `[FAIL]` lines
- Onboard LED (near USB connector) blinks 5 times
- Heap loop prints every 5 seconds

### Fail criteria and troubleshooting

| Failure message | Cause | Fix |
|---|---|---|
| No output on Serial Monitor | Wrong COM port or baud | Check Device Manager / `ls /dev/ttyUSB*`, set 115200 |
| `Failed to mount LittleFS` | Wrong partition scheme | Tools → Partition Scheme → `Default 4MB with spiffs` |
| `Free heap < 100KB` | Memory fragmentation or leak | Flash a fresh sketch, check for large static arrays |
| `NVS write failed` | NVS full from previous test | Flash → Tools → Erase All Flash Memory |
| No blink on LED | GPIO 2 wired differently on your board | Check your board's LED pin; some NodeMCU 38-pin use GPIO 2, some differ |

**Common mistakes:**
- Opening Serial Monitor before upload finishes (see garbage characters) — wait for "Done uploading"
- Using a charge-only USB cable (upload fails silently)
- Not pressing the BOOT button while uploading on boards without auto-reset circuitry

---

## Test 12 — LittleFS

**Hardware required:** ESP32, USB cable
**Hardware optional:** None
**Estimated time:** 10 minutes (rotation test writes ~50KB, takes ~5s)

### Wiring
```
[USB-A] ─────────────────── [ESP32 Micro-USB]
```

### Required libraries
- ArduinoJson 7.x (Library Manager)
- LittleFS — built into ESP32 Arduino core, no install needed

### Arduino IDE / PlatformIO setup
- Open `12_littlefs/12_littlefs.ino`
- **Critical:** Partition Scheme must be `Default 4MB with spiffs` (or any scheme with a SPIFFS/LittleFS partition ≥ 64KB)
- Upload → Serial Monitor at 115200

### Expected serial output
```
========================================
 MMS V2 — Test 12: LittleFS
========================================
[INFO] Auto-format enabled — first run will format partition if needed
[INFO] Note: Rotation test writes ~50KB of data — takes a few seconds

[INFO] === Phase 1: Mount ===
[PASS] LittleFS mounted (or auto-formatted and mounted)
[INFO] LittleFS: total=1520KB  used=0KB  free=1520KB
[PASS] LittleFS partition >= 64KB

[INFO] === Phase 2: File Write / Read / Verify ===
[INFO] Written 32 bytes to /test/hw_test.txt
[PASS] Read-back matches written content exactly
[PASS] File size matches content length

[INFO] === Phase 3: JSONL Append + Parse ===
[INFO]  line 1  t=1718600000  r=220002120  d=120
[INFO]  line 2  t=1718600300  r=220002121  d=150
[INFO]  line 3  t=1718600600  r=220002122  d=180
[INFO]  line 4  t=1718600900  r=220002123  d=210
[INFO]  line 5  t=1718601200  r=220002124  d=240
[INFO] Parsed 5 valid / 0 invalid lines
[PASS] All 5 JSONL lines written and parsed successfully

[INFO] === Phase 4: Partial-line Resilience ===
[INFO] Discarding partial/invalid line: {"t":9999,"r":"INCOMPLETE
[PASS] Partial line detected and discarded — valid lines intact

[INFO] === Phase 5: Log Rotation (50KB threshold) ===
[INFO] File reached 50KB — rotating...
[INFO] sessions.jsonl → sessions.old.jsonl
[INFO] Lines written before rotation: 412
[PASS] Rotation: old file renamed, current slot cleared
[PASS] Rotated file contains expected data volume (>= 50KB)

[INFO] === Phase 6: Delete + Free Space Recovery ===
[INFO] LittleFS: total=1520KB  used=51KB  free=1469KB
[INFO] Space recovered: 51KB
[PASS] Delete freed space — LittleFS garbage collection working
[PASS] All test files deleted successfully

[INFO] === Phase 7: Config File (revocation list format) ===
[INFO] Revoked UIDs in file: 2
[INFO]   A1B2C3D4
[INFO]   E5F60718
[PASS] revoked.json round-trip: 2 UIDs written and parsed correctly
[PASS] UID lookup in revocation list works correctly

========================================
 RESULT: 12/12 passed
 STATUS: PASS
========================================
```

### Expected display output
None.

### Pass criteria
All 12 checks print `[PASS]`. Rotation test completes (may take 5–10 seconds — the serial will be silent while writing).

### Fail criteria and troubleshooting

| Failure message | Cause | Fix |
|---|---|---|
| `LittleFS.begin() failed` | No LittleFS partition in scheme | Change partition scheme; re-upload |
| `Partition too small: 0KB` | Wrong partition scheme selected | Use `Default 4MB with spiffs` |
| `JSONL parse failed` | ArduinoJson version mismatch | Install ArduinoJson 7.x, not 5.x or 6.x |
| Rotation test hangs | Filesystem full before 50KB | Check partition size; ensure `formatOnFail=true` re-formatted |
| `Delete freed space` check fails | LittleFS GC needs remount | Cosmetic — re-run test after reset |

**Common mistakes:**
- Installing ArduinoJson 6.x instead of 7.x (API difference — `StaticJsonDocument` deprecated in 7.x but still works; confirm no compile errors)
- Not changing partition scheme before Test 01 (both tests 01 and 12 need it — do it once at the start)

---

## Test 10 — WiFi

**Hardware required:** ESP32, USB cable, 2.4 GHz WiFi access point
**Hardware optional:** Router with internet (for NTP validation)
**Estimated time:** 10 minutes

### Wiring
```
[USB-A] ─────────────────── [ESP32 Micro-USB]
```
No breadboard wiring. Internal antenna.

### Required libraries
- WiFi — built into ESP32 Arduino core
- time.h — built-in C standard library

### Arduino IDE / PlatformIO setup
1. Open `10_wifi/10_wifi.ino`
2. Edit the `#define` block at the top:
   ```cpp
   #define WIFI_SSID      "your_network_name"
   #define WIFI_PASSWORD  "your_password"
   ```
3. Upload → Serial Monitor at 115200

### Expected serial output
```
========================================
 MMS V2 — Test 10: WiFi
========================================
[INFO] Target SSID: tlmms
[INFO] Note: ESP32 supports 2.4 GHz only — not 5 GHz

[INFO] === Phase 1: Network Scan ===
[INFO] Found 8 networks:
[INFO]   -45 dBm  [AUTH]  tlmms
[INFO]   -67 dBm  [AUTH]  JIOFIBER_HOME
[INFO]   -71 dBm  [OPEN]  Guest
...
[PASS] Target SSID found in scan

[INFO] === Phase 2: Connection ===
[INFO] Connecting to "tlmms"...
..
[PASS] Connected in 2340 ms
[INFO] IP address  : 192.168.0.105
[INFO] Subnet mask : 255.255.255.0
[INFO] Gateway     : 192.168.0.1
[INFO] DNS         : 192.168.0.1
[INFO] BSSID (AP)  : AA:BB:CC:DD:EE:FF
[INFO] Channel     : 6
[INFO] RSSI: -48 dBm (EXCELLENT)
[PASS] Valid IP address assigned
[PASS] RSSI above minimum threshold (-80 dBm)

[INFO] === Phase 3: NTP Time Sync ===
[INFO] Configuring NTP — server: pool.ntp.org  GMT offset: 19800s (IST = UTC+5:30)
[INFO] Waiting for NTP sync.......
[PASS] NTP synced — 2026-06-16 14:32:18 IST
[PASS] NTP time is current (year >= 2024)

[INFO] === Phase 4: Disconnect / Reconnect ===
[INFO] Forcing disconnect...
[PASS] WiFi disconnected cleanly
[INFO] Reconnecting...
.
[PASS] Reconnect OK in 1820 ms
[INFO] RSSI: -48 dBm (EXCELLENT)
[PASS] IP address retained after reconnect

========================================
 RESULT: 8/8 passed
 STATUS: PASS
========================================
[INFO] Monitoring mode: printing RSSI and heap every 10s.
[LOOP] Connected  RSSI=-48dBm  heap=231456B  uptime=30s
```

### Expected display output
None.

### Pass criteria
- Target SSID appears in scan
- IP address assigned (not 0.0.0.0)
- RSSI > -80 dBm at bench location
- NTP year ≥ 2024
- Reconnect succeeds

### Fail criteria and troubleshooting

| Failure message | Cause | Fix |
|---|---|---|
| `No networks found` | Antenna issue or SPI conflict | Ensure no SPI devices connected; try different location |
| `Target SSID not found` | 5 GHz only network, or hidden SSID | Enable 2.4 GHz band on router; unhide SSID |
| `Connection failed after 15s` | Wrong password | Double-check `WIFI_PASSWORD` — case sensitive |
| `WiFi status code: 6` | Wrong password confirmed | Re-check password |
| `IP address is 0.0.0.0` | DHCP failed | Check router DHCP pool isn't exhausted; restart router |
| `NTP sync timed out` | No internet access | Verify router has internet; UDP port 123 may be blocked |
| `RSSI below -80 dBm` | Node too far from AP | Move closer; use directional antenna |

**Common mistakes:**
- Network is 5 GHz-only ("AC" or "AX" only mode) — ESP32 is 2.4 GHz only
- SSID has a space or special character — use double quotes in `#define`
- Testing in a room with WiFi but the router is 5 floors up (poor RSSI)

---

---

# Phase B — Network Validation

---

## Test 11 — MQTT

**Hardware required:** ESP32, USB cable, WiFi AP, Raspberry Pi with Mosquitto
**Hardware optional:** Echo responder script (for round-trip latency — see below)
**Estimated time:** 15 minutes

### Wiring
```
[USB-A] ─────────────────── [ESP32 Micro-USB]
(same as Test 10 — no breadboard wiring)
```

### Pre-requisites on Raspberry Pi
SSH into the RPi and confirm Mosquitto is running:
```bash
sudo systemctl status mosquitto
# Should show: Active: active (running)
```
If not installed:
```bash
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable --now mosquitto
```

**Optional — echo responder** (enables QoS round-trip latency measurement):
Open a second terminal on the RPi and run:
```bash
mosquitto_sub -h localhost -t mms/test/echo | \
  xargs -I{} mosquitto_pub -h localhost -t mms/test/response -m "{}"
```
Leave this running during the test.

### Required libraries
- PubSubClient by Nick O'Leary 2.8.x (Library Manager)
- WiFi — built-in

### Arduino IDE / PlatformIO setup
1. Open `11_mqtt/11_mqtt.ino`
2. Edit the `#define` block:
   ```cpp
   #define WIFI_SSID      "your_network_name"
   #define WIFI_PASSWORD  "your_password"
   #define MQTT_BROKER    "192.168.0.10"    // RPi static IP
   #define MQTT_PORT      1883
   ```
3. Upload → Serial Monitor at 115200

### Expected serial output
```
========================================
 MMS V2 — Test 11: MQTT
========================================
[INFO] Broker: 192.168.0.10:1883
[INFO] ClientID: mms_hwtest_mqtt
[INFO] Round-trip test requires echo responder on broker.
[INFO] On RPi: mosquitto_sub -t mms/test/echo | xargs -I{} mosquitto_pub -t mms/test/response -m {}

[INFO] === Phase 1: WiFi + Broker Connect ===
[INFO] Connecting WiFi to "tlmms"...
..
[PASS] WiFi connected  IP=192.168.0.105
[INFO] Connecting to MQTT broker 192.168.0.10:1883
.
[PASS] MQTT broker connected
[PASS] Subscribed to mms/test/response

[INFO] === Phase 2: QoS 0 Round-trip ===
[MQTT RX] topic=mms/test/response  payload=ping_0_12345
[RTT]  attempt 1/5  QoS0  rtt=18ms
[RTT]  attempt 2/5  QoS0  rtt=15ms
...
[PASS] QoS 0 RTT < 500ms on LAN

[INFO] === Phase 3: QoS 1 Publish ===
[INFO] QoS 1 topic publishes: 5/5
[PASS] All QoS 1 topic publishes acknowledged by broker

[INFO] === Phase 4: Retained Message Test ===
[INFO] Retained message published to mms/test/retained
[INFO] Disconnected. Reconnecting to verify retained delivery...
[PASS] Reconnect after retained test succeeded
[MQTT RX] topic=mms/test/retained  payload={"test":"retained_v2","ts":1}
[PASS] Retained message delivered on reconnect + subscribe
[INFO] Retained message cleared (empty payload published with retain=true)

[INFO] === Phase 5: Disconnect / Reconnect Timing ===
[INFO] State after disconnect: -1
[PASS] Reconnect in 380 ms
[PASS] Reconnect latency < 2s (acceptable for lab LAN)

========================================
 RESULT: 9/9 passed
 STATUS: PASS
========================================
[LOOP] Heartbeat published  {"uptime":45,"heap":215000,"rssi":-48}
```

### Expected display output
None.

### Pass criteria
- Broker connection established
- Retained message received on reconnect
- QoS round-trip < 500ms (if echo responder is running)
- All 5 QoS 1 publishes return `true`
- Reconnect in < 2s

### Fail criteria and troubleshooting

| Failure message | Cause | Fix |
|---|---|---|
| `MQTT broker connection failed` + state -2 | Wrong broker IP or port | `ping 192.168.0.10` from laptop to verify RPi reachable |
| State -4: `CONN_TIMEOUT` | Mosquitto not listening on 1883 | `sudo netstat -tlnp \| grep 1883` on RPi |
| State 5: `UNAUTH` | Mosquitto requires password | Add `allow_anonymous true` to `/etc/mosquitto/mosquitto.conf`, restart |
| `Retained message not received` | Mosquitto persistence off | Add `persistence true` to mosquitto.conf |
| `No echoes received` | Echo responder not running | Run the `mosquitto_sub \| xargs` command on RPi |
| Reconnect > 2s | RPi under load | Acceptable — warn only, not fail |

**Common mistakes:**
- Using RPi's WiFi IP instead of its Ethernet IP (WiFi IP can change; use static Ethernet)
- Mosquitto running on localhost only — check `bind_address` is not set or is `0.0.0.0`
- Forgetting to start the echo responder (Phase 2 will print "No echoes" — not a fail, just informational)
- Confusing PubSubClient's `publish()` QoS: PubSubClient publish is always QoS 0 at the library level; QoS 1 delivery is handled by the `subscribe()` + retained test

---

---

# Phase C — I2C

---

## Test 05 — OLED 128×32 I2C

**Hardware required:** ESP32, SSD1306 128×32 I2C OLED, jumper wires
**Hardware optional:** None
**Estimated time:** 10 minutes

### Wiring

```
ESP32 NodeMCU 38-pin          SSD1306 128×32 I2C OLED
                              (4-pin module)
─────────────                 ─────────────────────────
3V3  (pin 2)  ─────────────── VCC
GND  (pin 1)  ─────────────── GND
GPIO 21       ─────────────── SDA
GPIO 22       ─────────────── SCL

I2C address: 0x3C (default on most modules)
No RST pin connection needed — OLED_RESET = -1 in firmware
```

**Breadboard layout:**
```
ESP32                                    OLED 128×32
 3V3 ─── [RED wire]   ────────────────── VCC
 GND ─── [BLACK wire] ────────────────── GND
GPIO21 ── [BLUE wire]  ────────────────── SDA
GPIO22 ── [YELLOW wire]────────────────── SCL
```

### Required libraries
- Adafruit SSD1306 2.5.x
- Adafruit GFX Library 1.11.x
- Adafruit BusIO (auto-installed with SSD1306)

### Arduino IDE / PlatformIO setup
- Open `05_oled32_i2c/05_oled32_i2c.ino`
- Upload → Serial Monitor at 115200
- **Watch both the serial output AND the OLED screen simultaneously**

### Expected serial output
```
========================================
 MMS V2 — Test 05: OLED 128×32 I2C
========================================
[INFO] I2C scan on SDA=21 SCL=22 at 400kHz...
[INFO] I2C device found at address 0x3C
[PASS] SSD1306 at 0x3C — expected address confirmed

[INFO] Initializing SSD1306 128×32 I2C...
[PASS] SSD1306 128×32 initialized

[INFO] === Page 1: Status Strip IDLE ===
[INFO] === Page 2: Status Strip SESSION ===
[INFO] === Page 3: Full Fill (all pixels on) ===
[INFO] === Page 4: Vertical Stripe Pattern ===
[INFO] === Page 5: OFFLINE Status ===

========================================
 RESULT: 2/2 passed
 STATUS: PASS (visual checks required)
========================================
```

### Expected display output

| Page | What you should see on the 128×32 OLED |
|---|---|
| 1 (IDLE) | Left: WiFi dot + MQTT dot. Center: "SOLDERING STA 1". Right: "14:32" or "OFFLINE" |
| 2 (SESSION) | Left: filled WiFi + MQTT dots. Center: "SOLDERING STA 1". Right: "14:32" |
| 3 (Fill) | All 128×32 pixels illuminated — solid white rectangle |
| 4 (Vertical stripes) | Alternating 4px-wide vertical black/white bars |
| 5 (Offline) | Small WiFi hollow dot + "OFFLINE" + machine name |

Each page displays for 3 seconds before advancing.

### Pass criteria
- I2C scan finds device at `0x3C`
- OLED initialises without error
- All 5 pages are visible and match the descriptions above
- No garbled or blank screens between pages

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
OLED is completely blank?
  ├── Serial shows "I2C device found at 0x3C" → SSD1306.begin() failed
  │     → Check library version (SSD1306 2.5.x). Try swapping SDA/SCL wires.
  ├── Serial shows "No I2C devices found"
  │     → Check VCC (multimeter: should be 3.3V on VCC pin)
  │     → Check SDA/SCL on correct GPIO (21 and 22)
  │     └── Some cheap modules need 5V VCC — try 5V (Vin) instead of 3V3
  └── Upload failed → Check COM port

I2C scan finds device at 0x3D instead of 0x3C?
  → Module has address select solder bridge. Change address in .ino:
    Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, OLED_RESET, 400000, 400000);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3D)

Display shows white noise / garbage pixels?
  → Power issue. Add 10µF capacitor across VCC–GND on breadboard.

First page shows but subsequent pages are blank?
  → clearDisplay() not called — library version issue. Upgrade Adafruit SSD1306.
```

| Failure message | Cause | Fix |
|---|---|---|
| `No I2C devices found` | Wrong pins or no power | Verify GPIO 21=SDA, 22=SCL; check VCC |
| `SSD1306 init failed` | Address mismatch | Try `0x3D` in init call |
| All pages blank | Wrong height (32 vs 64) | Confirm display is 32-pixel height in constructor |

**Common mistakes:**
- Using a 128×64 I2C OLED instead of the 128×32 model (display height mismatch — firmware halves the height)
- Swapping SDA and SCL (device found at scan but display looks wrong)
- VCC on 5V pin instead of 3.3V (some modules overheat; most work on both — check module datasheet)

---

---

# Phase D — SPI Bus

> **Note:** Both Test 04 (OLED SPI) and Test 02 (MFRC522) share the same SPI bus (GPIO 18/19/23). Wire the OLED first, validate it alone, then add the MFRC522.

---

## Test 04 — OLED 128×64 SPI

**Hardware required:** ESP32, SSD1306 128×64 SPI OLED (7-pin), jumper wires
**Hardware optional:** None
**Estimated time:** 10 minutes

### Wiring

```
ESP32 NodeMCU 38-pin          SSD1306 128×64 SPI OLED
                              (7-pin SPI module)
─────────────                 ─────────────────────────
3V3  (pin 2)  ─────────────── VCC
GND  (pin 1)  ─────────────── GND
GPIO 18       ─────────────── D0 (SCK / CLK)
GPIO 23       ─────────────── D1 (MOSI / SDA)
GPIO 17       ─────────────── DC  (data/command select)
GPIO 16       ─────────────── CS  (chip select)
GPIO  4       ─────────────── RES (reset)
```

**7-pin module label variants** — these are the same signal:
- `D0` = `CLK` = `SCK` = clock → GPIO 18
- `D1` = `MOSI` = `SDA` = data → GPIO 23
- `DC` = `A0` = data/command → GPIO 17
- `CS` = `SS` = chip select → GPIO 16
- `RES` = `RST` = reset → GPIO 4

### Required libraries
- Adafruit SSD1306 2.5.x
- Adafruit GFX Library 1.11.x

### Arduino IDE / PlatformIO setup
- Open `04_oled64_spi/04_oled64_spi.ino`
- Upload → Serial Monitor at 115200
- Watch the display and serial simultaneously

### Expected serial output
```
========================================
 MMS V2 — Test 04: OLED 128×64 SPI
========================================
[INFO] SPI OLED — DC=17 RST=4 CS=16
[INFO] Shared SPI bus: SCK=18 MOSI=23
[PASS] SSD1306 128×64 SPI initialized

[INFO] === Page 1: System Info ===
[INFO] === Page 2: Full Fill (all pixels) ===
[INFO] === Page 3: Horizontal Stripes ===
[INFO] === Page 4: MMS V2 IDLE Screen ===
[INFO] === Page 5: SESSION_ACTIVE Screen ===
[INFO] === Page 6: OTA Progress Animated ===
[INFO] === Page 7: Checkerboard Pattern ===

========================================
 RESULT: 1/1 passed
 STATUS: PASS (visual confirmation required for all pages)
========================================
[INFO] Looping pages continuously. Press reset to re-run.
```

### Expected display output

| Page | What you should see |
|---|---|
| 1 (System Info) | "MMS V2 Test 04", "OLED 128x64 SPI", "SCK=18 MOSI=23 DC=17 CS=16 RST=4" |
| 2 (Fill) | All 128×64 pixels on — solid white rectangle, 3s |
| 3 (H-stripes) | Alternating horizontal black/white stripes, 3s |
| 4 (IDLE screen) | Large "Tinkerers' Lab" header, "Soldering Sta 1", "Tap card to start" |
| 5 (SESSION screen) | "Roll: 220002123", machine name, session timer |
| 6 (OTA progress) | "Updating FW...", percentage counter 0→100%, progress bar animating |
| 7 (Checkerboard) | 8×8 pixel checkerboard pattern |

Then loops back to page 1.

### Pass criteria
- `[PASS] SSD1306 128×64 SPI initialized` prints
- All 7 pages are visible, content matches descriptions
- OTA progress bar animates smoothly (page 6)
- Loop runs continuously without lockup

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
Display completely blank (no backlight)?
  ├── Module may have no backlight (OLED does not have separate backlight LED)
  │     → Normal — OLED pixels are self-illuminating
  └── SSD1306.begin() returned false
        → Check all 5 signal wires (DC, CS, RST especially)
        → Check VCC = 3.3V (use multimeter)

Display shows static/garbage on first boot?
  → RST not connected or connected wrong GPIO
  → Verify GPIO 4 → RES pin

Some pixels stuck on?
  → Power issue — add 10µF cap across VCC-GND; try shorter jumper wires

Pages advance but content looks corrupted?
  → Wrong CS pin — another SPI device asserting CS simultaneously
  → Disconnect MFRC522 if already wired
```

| Failure message | Cause | Fix |
|---|---|---|
| `SSD1306 init failed` | SPI pins wrong or disconnected | Verify pin assignments one by one with multimeter |
| Display works but MFRC522 stops working | SPI bus conflict (CS pins) | Each device must have its own CS; verify GPIO 16 ≠ GPIO 5 |

**Common mistakes:**
- Confusing the 4-pin I2C OLED with the 7-pin SPI OLED (different module, different test)
- Connecting D1 (MOSI) to GPIO 19 (MISO) — SPI data direction matters
- Missing the RES pin connection (causes garbage on screen)

---

## Test 02 — MFRC522 RFID Reader

**Hardware required:** ESP32, MFRC522 module, MIFARE Classic 1K card, jumper wires
**Hardware optional:** OLED from Test 04 (can stay wired — shares SPI bus)
**Estimated time:** 15 minutes

### Wiring

```
ESP32 NodeMCU 38-pin          MFRC522 RFID Module
─────────────                 ──────────────────────
3V3  (pin 2)  ─────────────── 3.3V (VCC — NOT 5V)
GND  (pin 1)  ─────────────── GND
GPIO 18       ─────────────── SCK
GPIO 19       ─────────────── MISO
GPIO 23       ─────────────── MOSI
GPIO  5       ─────────────── SDA (SS / CS)
GPIO 27       ─────────────── RST
              (IRQ — leave unconnected)
```

**MFRC522 runs at 3.3V — never connect VCC to 5V.**

**If SPI OLED (Test 04) is still wired**, it shares GPIO 18/19/23 (SCK/MISO/MOSI). Leave those connected. Add only the MFRC522-specific wires:
- GPIO 5 → SDA (MFRC522 CS — different from OLED CS on GPIO 16)
- GPIO 27 → RST
- 3V3 → 3.3V
- GND → GND

### Required libraries
- MFRC522 by miguelbalboa 1.4.x (Library Manager)

### Arduino IDE / PlatformIO setup
- Open `02_mfrc522/02_mfrc522.ino`
- Upload → Serial Monitor at 115200
- Have the MIFARE 1K card ready to tap

### Expected serial output (Phase 1 — module test, before any card tap)
```
========================================
 MMS V2 — Test 02: MFRC522 RFID Reader
========================================
[INFO] SPI bus: SCK=18 MISO=19 MOSI=23
[INFO] MFRC522: SS=5 RST=27

[INFO] === Phase 1: SPI Communication ===
[INFO] VersionReg: 0x92
[PASS] VersionReg = 0x91 or 0x92 (genuine MFRC522)

[INFO] === Phase 2: Self-test ===
[PASS] MFRC522 self-test passed
[INFO] MFRC522 re-initialized after self-test

========================================
[INFO] Tap a MIFARE Classic 1K card...
```

### Expected serial output (after card tap)
```
[EVENT] Card detected
[INFO] Card UID: A1 B2 C3 D4   (4 bytes)
[PASS] UID length: 4 bytes
[INFO] PICC type: MIFARE 1K
[PASS] PICC type is MIFARE 1K

[INFO] Authenticating Sector 0 Block 0 with default key (FF FF FF FF FF FF)...
[PASS] Authentication succeeded
[INFO] Block 0 raw data: 01 23 45 67 D1 08 04 00 62 63 64 65 66 67 68 69
[PASS] Block 0 read successful
[INFO] (This is the manufacturer block — unique per card)

[PASS] Card halted cleanly
```

### Expected display output
None — this sketch uses serial only.

### Pass criteria
- VersionReg reads `0x91` or `0x92` (any other value = fake or dead chip)
- Self-test passes
- Card UID reads in 4 bytes (MIFARE 1K)
- PICC type = `MIFARE 1K`
- Block 0 authentication with default key succeeds
- 16 bytes of block 0 data printed

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
VersionReg = 0x00 or 0xFF?
  ├── SPI not communicating
  │     → Check SCK/MISO/MOSI wiring (all three required)
  │     → Check SS = GPIO 5 (not GPIO 15 or 4)
  └── MFRC522 powered incorrectly
        → Verify 3V3 on VCC — NOT 5V (module will be warm to touch if 5V)

VersionReg = 0x88 or similar?
  → Counterfeit MFRC522 chip — many clones work despite wrong version register
  → Continue test; if self-test fails with clone, this is expected behaviour

Self-test fails?
  ├── Counterfeit chip (common) — try another module
  └── SPI bus shared and CS of OLED not held HIGH
        → Disconnect OLED temporarily to isolate

Card not detected (times out)?
  ├── Card too far — tap flat against module (< 2cm)
  ├── Wrong card type — must be MIFARE Classic 1K, not NTAG215/Ultralight
  └── RST pin issue — check GPIO 27 → RST

Authentication fails with default key?
  → Card was previously programmed with custom sector key (V1 kiosk may have done this)
  → Use a blank factory card; or use V2 Data_reader.ino to inspect sector 0
```

| Failure message | Cause | Fix |
|---|---|---|
| `VersionReg = 0x00` | SPI not connected | Check all 5 SPI wires |
| `VersionReg = 0xFF` | No chip detected / wrong VCC | Verify 3.3V on VCC |
| `Self-test FAILED` | Counterfeit chip | Expected on clones — proceed anyway |
| `Timeout waiting for card` | Wrong card type | Use MIFARE Classic 1K only |
| `Authentication failed` | Card pre-programmed | Use factory-blank card |

**Common mistakes:**
- Connecting VCC to 5V — the module may work but can fail intermittently; damages chip over time
- Using a bank card, hotel key, or transit card (these are not MIFARE Classic 1K)
- Forgetting to call `PCD_Init()` after self-test (the self-test leaves the module in a bad state — the firmware handles this, but check if copy-pasting code)

---

---

# Phase E — Digital GPIO

---

## Test 03 — HC89 Slot Sensor

**Hardware required:** ESP32, HC89 optical slot sensor, jumper wires, thin card/paper strip
**Hardware optional:** Oscilloscope (for debounce verification — not required)
**Estimated time:** 15 minutes (requires 10 insertion/removal cycles)

### Wiring

```
ESP32 NodeMCU 38-pin          HC89 Optical Slot Sensor
─────────────                 ─────────────────────────
3V3  (pin 2)  ─────────────── VCC  (or 5V — check module)
GND  (pin 1)  ─────────────── GND
GPIO 32       ─────────────── OUT  (signal)

Note: GPIO 32 is configured as INPUT_PULLUP in firmware.
If the module has its own pull-up resistor, the internal pull-up is redundant but harmless.
```

**HC89 output logic:**
- Slot EMPTY (no card): OUT = HIGH (LED beam unbroken → open-collector HIGH)
- Card INSERTED (beam blocked): OUT = LOW

**Test card:** Use any thin plastic card, cardboard strip (~1mm), or the MIFARE card. The slot is typically 1–2mm wide.

### Required libraries
None — digital GPIO only.

### Arduino IDE / PlatformIO setup
- Open `03_hc89_slot_sensor/03_hc89_slot_sensor.ino`
- Upload → Serial Monitor at 115200
- Follow the guided insertion/removal cycles

### Expected serial output
```
========================================
 MMS V2 — Test 03: HC89 Slot Sensor
========================================
[INFO] HC89 on GPIO 32 (INPUT_PULLUP)
[INFO] LOW = card present, HIGH = slot empty (active LOW)

[INFO] === Phase 1: Initial State ===
[INFO] GPIO 32 reading: HIGH (slot empty — correct if no card)
[PASS] Initial state HIGH confirmed

[INFO] === Phase 2: Guided 10-cycle test ===
[INFO] Insert and remove the card 10 times.
[INFO] Insert slowly — full insertion = card fully blocks IR beam.
[INFO] Remove completely — card fully clear of slot.
[STATE] empty  insertions=0  removals=0  false_triggers=0

[EVENT] INSERTED  debounce_ms=50  → CARD PRESENT
[STATE] present  insertions=1  removals=0  false_triggers=0
[EVENT] REMOVED   debounce_ms=50  → SLOT EMPTY
[STATE] empty  insertions=1  removals=1  false_triggers=0
... (repeat 10 times) ...

[INFO] === Validation Summary (after 10 cycles) ===
[INFO] Insertions : 10
[INFO] Removals   : 10
[INFO] False triggers: 0
[PASS] 10 insertions detected
[PASS] 10 removals detected
[PASS] False trigger count < 5

========================================
 RESULT: 4/4 passed
 STATUS: PASS
========================================
```

### Expected display output
None.

### Pass criteria
- Initial state is HIGH (empty slot) when no card is present
- 10 insertion events detected with `[EVENT] INSERTED`
- 10 removal events detected with `[EVENT] REMOVED`
- False triggers < 5 across all 10 cycles

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
Initial reading is LOW with empty slot?
  ├── Module powered by 5V but logic out at 5V — GPIO 32 is 3.3V tolerant but check
  ├── OUT wire connected to wrong GPIO (not 32)
  └── Module defective — swap module

No [EVENT] on card insertion?
  ├── Card not fully inserted (partial block doesn't trigger)
  │     → Push card further into slot
  ├── OUT not connected to GPIO 32
  └── Module not powered — check VCC

Constant false triggers (jitter)?
  ├── Debounce time too short for this module
  │     → Increase DEBOUNCE_MS define in sketch from 50 to 100
  └── Power supply instability — add 10µF cap across VCC-GND

Insertions count but removals don't?
  → Card removal too fast (under debounce window)
  → Remove card slowly and hold out for > 100ms
```

| Failure message | Cause | Fix |
|---|---|---|
| `Initial state LOW (card not present)` | Wiring inverted | Check OUT → GPIO 32; check GND shared |
| False triggers > 5 | Mechanical vibration or power noise | Increase debounce; add bypass cap |
| Only 9/10 insertions | Partial insertion | Insert fully; ensure card blocks beam |

**Common mistakes:**
- Using a card that's too thick (> 2mm) — it won't insert cleanly
- Inserting card at an angle — must be straight on
- Counting partial insertions (card halfway) — must be fully through the slot

---

## Test 06 — Mechanical Relay

**Hardware required:** ESP32, single-channel 5V relay module, multimeter, jumper wires
**Hardware optional:** Buzzer or LED (to hear/see the relay switching)
**Estimated time:** 10 minutes

### Wiring

```
ESP32 NodeMCU 38-pin          Relay Module (5V single channel)
─────────────                 ─────────────────────────────────
Vin  (5V USB) ─────────────── VCC   (5V for relay coil)
GND           ─────────────── GND
GPIO 26       ─────────────── IN

Multimeter probes on relay terminals:
  COM probe A ──────────────── COM terminal
  COM probe B ──────────────── NO terminal
  (Set multimeter to CONTINUITY or OHM mode)
```

**SAFETY:** Do not connect mains voltage (240V AC) to the relay terminals during this test. Multimeter continuity mode only.

**Relay module variants:**
- Most blue relay modules (with optocoupler): active HIGH — IN HIGH = relay ON ← default
- Some relay modules (without optocoupler): active LOW — IN LOW = relay ON → send `l` command

### Required libraries
None.

### Arduino IDE / PlatformIO setup
- Open `06_relay_mechanical/06_relay_mechanical.ino`
- Upload → Serial Monitor at 115200
- Set Serial Monitor line ending to "Newline" to send single-character commands

### Expected serial output
```
========================================
 MMS V2 — Test 06: Mechanical Relay
========================================
[PASS] GPIO 26 starts LOW (safe state on boot)
[INFO] In ACTIVE HIGH mode this means relay is OFF — correct

[INFO] SAFETY: DO NOT connect mains voltage.
[INFO] Use multimeter in CONTINUITY mode on COM↔NO terminals.
...
[INFO] Default mode: ACTIVE HIGH (IN HIGH = relay ON)
[INFO] Commands (send via Serial Monitor at 115200 baud):
[INFO]   h = active HIGH mode
[INFO]   l = active LOW mode
[INFO]   o = relay ON
[INFO]   f = relay OFF
[INFO]   p = 500ms pulse
[INFO]   c = 10-cycle test (100ms on/off)
[INFO]   s = print status
[INFO]   ? = this help
[STATE] Relay OFF  GPIO26=LOW   mode=active_HIGH
```

**Interactive test sequence — send these commands one at a time:**

| Command | Expected output | Expected sound | Multimeter |
|---|---|---|---|
| `o` | `[ACTION] Relay ON (GPIO26=HIGH)` | CLICK | Beep / < 1Ω |
| `f` | `[ACTION] Relay OFF (GPIO26=LOW)` | CLICK | Silent / open |
| `p` | ON for 500ms then OFF, two clicks | CLICK … CLICK | Beep briefly |
| `c` | 10 × ON/OFF, 20 clicks | 20 clicks in ~2s | 10 beeps |

### Expected display output
None.

### Pass criteria
- GPIO 26 reads LOW on boot (before any command)
- Audible click on each ON and OFF transition
- Multimeter beeps (< 1Ω) when relay is ON
- Multimeter shows open circuit when relay is OFF
- 10-cycle test completes without GPIO errors
- `[PASS]` after each test

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
Relay clicks on power-up (before any command)?
  → Relay module is ACTIVE LOW — send 'l' to switch mode
  → This is normal for some modules — active HIGH is default in V2 firmware

No click on 'o' command?
  ├── VCC wired to 3.3V instead of 5V (Vin) — relay coil needs 5V
  ├── GPIO 26 not wired to IN
  ├── Module needs active LOW — send 'l' first, then 'o'
  └── Module defective

Multimeter doesn't beep but relay clicks?
  → Probes on wrong terminals — use COM + NO (normally open)
  → If on COM + NC (normally closed): beep when OFF, silent when ON

GPIO 26 fails to go HIGH?
  ├── Pin conflict — check no other wire on GPIO 26
  └── ESP32 damaged GPIO — try another GPIO by modifying PIN_RELAY define
```

| Failure message | Cause | Fix |
|---|---|---|
| `GPIO 26 not LOW on boot` | Relay module pulling GPIO | Disconnect relay, re-flash, recheck GPIO state |
| No clicks on any command | 5V not on VCC | Check Vin pin (should read 4.8–5V from USB) |
| `10-cycle test: GPIO errors` | Timing too fast for module | Increase CYCLE_ON_MS/CYCLE_OFF_MS in sketch |

**Common mistakes:**
- Using 3.3V instead of 5V for relay VCC (coil won't energise reliably at 3.3V)
- Multimeter probes on COM+NC instead of COM+NO (logic inverted — beeps when OFF)
- Sending multiple characters in Serial Monitor (e.g. "on\n") — sketch expects single character

---

## Test 07 — Solid State Relay (SSR)

**Hardware required:** ESP32, SSR module (e.g. Fotek SSR-25DA), multimeter, jumper wires
**Hardware optional:** LED with current-limiting resistor (to see switching without mains)
**Estimated time:** 10 minutes

### Wiring

```
ESP32 NodeMCU 38-pin          Solid State Relay
─────────────                 ──────────────────────────────
GPIO 26       ─────────────── INPUT+  (control positive)
GND           ─────────────── INPUT-  (control negative)

Multimeter probes on SSR LOAD terminals:
  Probe A ────────────────────── LOAD1
  Probe B ────────────────────── LOAD2
  (Set to OHM or CONTINUITY mode)
```

**No VCC wire needed** — SSR control input draws < 5mA from GPIO 26 directly (3.3V is above the minimum control voltage for most SSRs rated 3–32V DC input).

**SAFETY:** LOAD1 and LOAD2 are the AC mains terminals. Leave them completely disconnected from any AC source. Multimeter only.

**Check before wiring:** Read the SSR label. Control input specification should say something like `3–32V DC`. If it says `4–32V DC`, the 3.3V GPIO may be marginal — test and observe the STATUS LED on the SSR body.

### Required libraries
None.

### Arduino IDE / PlatformIO setup
- Open `07_relay_ssr/07_relay_ssr.ino`
- Upload → Serial Monitor at 115200
- Same single-character command interface as Test 06

### Expected serial output
```
========================================
 MMS V2 — Test 07: Solid State Relay (SSR)
========================================
[PASS] GPIO 26 starts LOW — SSR off on boot (safe state)

[SAFETY] DO NOT connect mains voltage during this test.
[SAFETY] Use multimeter in OHM or CONTINUITY mode on LOAD terminals.

── SSR vs Mechanical Relay ────────────────────────────────────────
[INFO] No click sound — use LED on SSR and multimeter to confirm
[INFO] 3.3V GPIO drives INPUT+ directly — no extra transistor needed
[INFO] AC SSR: zero-cross switch → ON within half cycle (~10ms@50Hz)
[INFO] Off-state leakage: 1–3V on LOAD terminals is NORMAL for SSR
[INFO] If load terminals measure full supply voltage when 'off',
[INFO] SSR is welded — replace immediately
──────────────────────────────────────────────────────────────────────
...
[INFO] Measuring GPIO 26 output voltage:
[INFO]   GPIO HIGH — measure voltage at SSR INPUT+
[INFO]   Expected: 2.8–3.3V. SSR control spec: 3–32V DC.
[INFO]   Some SSRs need 5V minimum — check datasheet.
[INFO]   GPIO LOW — SSR should deactivate.
```

**Interactive test** — same commands as Test 06 (`o`, `f`, `p`, `c`).

Key difference from Test 06: **no audible click**. Use the SSR's status LED and multimeter.

### Expected display output
None.

### Pass criteria
- GPIO 26 is LOW on boot
- SSR status LED illuminates when `o` command sent
- SSR status LED extinguishes when `f` command sent
- Multimeter shows near-0Ω between LOAD terminals when ON
- Multimeter shows open (or 1–3V leakage — normal) when OFF
- 10-cycle test completes, SSR LED flashes 10 times

### Fail criteria and troubleshooting

| Failure message | Cause | Fix |
|---|---|---|
| SSR status LED doesn't light | GPIO 26 not connected to INPUT+ | Check wiring |
| SSR LED lights but LOAD shows open | AC SSR on DC load (wrong SSR type) | AC SSRs need AC on LOAD to switch. Use DC SSR for DC loads. |
| Multimeter shows full voltage when OFF | SSR welded (shorted output) | Replace SSR immediately — do not use |
| `GPIO 26 not LOW on boot` | SSR module has pull-up on input | Check if SSR module has onboard components vs bare SSR |

**Common mistakes:**
- Measuring leakage (1–3V) on LOAD when OFF and concluding it's "stuck on" — this is normal for triacs
- Using an AC SSR on a DC circuit (won't work — use an appropriate SSR type)
- Connecting INPUT- to 3.3V instead of GND (polarity matters on DC SSR control input)

---

## Test 08 — WS2812 RGB LED

**Hardware required:** ESP32, WS2812B LED (1 pixel minimum), 470Ω resistor, 100µF capacitor, jumper wires
**Hardware optional:** 5V power supply (USB can source 5V via Vin)
**Estimated time:** 15 minutes (visual inspection of all color sequences)

### Wiring

```
ESP32 NodeMCU 38-pin          WS2812B LED
─────────────                 ─────────────────────────────────
Vin  (5V USB) ──[100µF+]───── VCC (+5V)
              ──[100µF-]───── (connects to GND below)
GND           ─────────────── GND
GPIO 33 ──[470Ω]──────────── DIN (data in)

[100µF] capacitor: + leg to VCC, – leg to GND
         Place it as close to the LED module as possible

[470Ω] resistor: in series between GPIO 33 and DIN
         Protects against ringing on long wires
```

**Single pixel from a strip:** Cut 1 LED from a WS2812B strip. The strip has 3 contacts: VCC, GND, DATA IN (→ left side). The right side is DATA OUT — leave unconnected.

**For module boards** (pre-mounted LED): Board usually provides VCC, GND, DIN pins. Same wiring, just using board connectors.

### Required libraries
- Adafruit NeoPixel 1.12.x (Library Manager)

### Arduino IDE / PlatformIO setup
- Open `08_ws2812_led/08_ws2812_led.ino`
- If colors appear wrong (red shows as green), change `LED_TYPE` in sketch:
  ```cpp
  #define LED_TYPE  (NEO_RGB + NEO_KHZ800)   // change from NEO_GRB
  ```
- Upload → Serial Monitor at 115200

### Expected serial output
```
========================================
 MMS V2 — Test 08: WS2812 RGB LED
========================================
[INFO] GPIO 33 → 470Ω → DIN   VCC=5V  NUM_LEDS=1
[INFO] LED type: NEO_GRB (change to NEO_RGB if colors wrong)
[SAFETY] Check: 100µF cap across VCC-GND at LED module
[SAFETY] Check: 5V on VCC pin (not 3.3V)

[INFO] Brief white flash — confirm LED is wired and powered
[INFO] === Basic Color Test (2s per color) ===
[INFO] RED   (255,   0,   0)
[INFO] GREEN (  0, 255,   0)
...
[PASS] Basic color sequence complete — confirm each color visually
[PASS] Color wheel sweep complete — confirm full hue spectrum with no gaps
[PASS] Brightness sweep — confirm smooth fade, no abrupt jumps
[INFO] === MMS V2 State Color Simulation ===
[PASS] IDLE: white breathing visible
[PASS] SESSION_ACTIVE: solid green
[PASS] TIMEOUT_WARNING: yellow/orange alternation visible
[PASS] ACCESS_DENIED: 3 red flashes
[PASS] MACHINE_DISABLED: dim purple visible
[PASS] OTA_UPDATE: blue pulse visible

 STATUS: PASS if all colors and animations were visible correctly
```

### Expected display output
The LED itself (no OLED in this test):

| Sequence | What you see on the LED |
|---|---|
| Power-on flash | Brief white half-brightness, then off |
| Red | Bright red, 2s |
| Green | Bright green, 2s |
| Blue | Bright blue, 2s |
| White | All-channels full, 2s |
| Orange | Orange-yellow, 2s |
| Yellow | Yellow, 2s |
| Off | Dark, 2s |
| Color wheel | Full spectrum sweep: red→yellow→green→cyan→blue→magenta→red |
| Brightness sweep | Red fading from black → full → black |
| IDLE (MMS) | Slow white pulsing/breathing |
| SESSION_ACTIVE | Solid green |
| TIMEOUT_WARNING | Alternating yellow ↔ orange every 500ms |
| ACCESS_DENIED | 3 fast red flashes |
| MACHINE_DISABLED | Dim purple |
| OTA_UPDATE | Blue pulsing |
| Loop | White slow breathing (IDLE) |

### Pass criteria
- LED lights on power-on flash (proves wiring is correct)
- Each of 7 basic colors is distinctly visible
- Color wheel shows continuous hue transition with no color stuck/skipped
- Brightness sweep is smooth (no sudden jump from 0 to bright)
- All 6 MMS state animations are visually distinct

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
LED doesn't light at all (including white flash)?
  ├── VCC not 5V — check Vin pin with multimeter (should be 4.8–5.2V)
  ├── DIN not connected (check 470Ω resistor orientation — it's not polar)
  ├── GND not connected
  └── LED may be damaged from prior 5V mis-wiring — swap LED

LED lights but shows wrong color (red = green)?
  → Change LED_TYPE in sketch from NEO_GRB to NEO_RGB and re-upload

LED flickers or shows random colors?
  ├── Missing 100µF capacitor → add cap across VCC-GND near LED
  ├── Long data wire without resistor → ensure 470Ω is in series
  └── 3.3V on VCC (not 5V) → check VCC = Vin (USB 5V), not 3V3

Breathing animation is choppy?
  → Normal on ESP32 with 16ms delay — should look smooth on eye

Color wheel has gap (one color missing)?
  → NEO_GRB vs NEO_RGB mismatch — change LED_TYPE
```

| Failure | Cause | Fix |
|---|---|---|
| No light | VCC on 3.3V | Move VCC wire from 3V3 pin to Vin pin |
| Wrong colors | GRB vs RGB byte order | Change `LED_TYPE` in sketch |
| Flickering | No decoupling cap | Add 100µF across VCC-GND |
| Very dim | VCC at 3.3V | WS2812 needs 5V for correct brightness |

**Common mistakes:**
- Powering from 3.3V (LED will be very dim and color-incorrect)
- No 470Ω resistor (LED still works usually, but is more susceptible to damage from ringing on long wires)
- Connecting DATA OUT (right side of strip) instead of DATA IN (left side)

---

---

# Phase F — Interrupt GPIO

---

## Test 09 — Rotary Encoder EC11

**Hardware required:** ESP32, EC11 rotary encoder (with push-button), 2× 10kΩ resistors, jumper wires
**Hardware optional:** EC11 breakout board (has pull-ups onboard — no external resistors needed)
**Estimated time:** 15 minutes (guided interaction)

### Wiring

**CRITICAL:** GPIO 34 and GPIO 35 on ESP32 are **input-only pins** with **no internal pull-up resistors**. External 10kΩ resistors to 3V3 are mandatory. Without them, the encoder will generate random counts.

```
ESP32 NodeMCU 38-pin          EC11 Rotary Encoder
─────────────                 ──────────────────────────────
3V3  (pin 2)  ─────────────── (+) VCC (if module has VCC pin)
GND  (pin 1)  ─────────────── (-) GND / C (common)
GPIO 34       ─────────────── CLK (A) — also add pull-up below
GPIO 35       ─────────────── DT  (B) — also add pull-up below
GPIO 25       ─────────────── SW  (button) — internal pull-up in firmware

External pull-up resistors (MANDATORY for GPIO 34 and 35):
  10kΩ from GPIO 34 → 3V3   (CLK pull-up)
  10kΩ from GPIO 35 → 3V3   (DT  pull-up)
```

**Breadboard pull-up layout:**
```
3V3 rail ──┬──[10kΩ]──┬── GPIO 34 (CLK)
           │           └── EC11 CLK pin
           └──[10kΩ]──┬── GPIO 35 (DT)
                       └── EC11 DT  pin

GND rail ──── EC11 GND/C pin
GPIO 25  ──── EC11 SW  pin  (no external resistor needed)
```

**If using a breakout board** with VCC/GND/CLK/DT/SW pins:
- VCC → 3V3
- GND → GND
- CLK → GPIO 34
- DT  → GPIO 35
- SW  → GPIO 25
- External pull-ups are likely already on the board — check at Phase 1 of test

### Required libraries
None — ISR + GPIO only.

### Arduino IDE / PlatformIO setup
- Open `09_rotary_encoder/09_rotary_encoder.ino`
- Upload → Serial Monitor at 115200
- **Important:** Keep Serial Monitor open while interacting with the encoder

### Expected serial output (startup)
```
========================================
 MMS V2 — Test 09: Rotary Encoder (EC11)
========================================
[INFO] CLK=GPIO34  DT=GPIO35  SW=GPIO25
[INFO] GPIO 34/35 are INPUT-ONLY — no internal pull-ups
[INFO] External 10kΩ pull-ups required from GPIO 34/35 to 3V3

[INFO] GPIO 34 (CLK) at rest: HIGH (pull-up OK)
[INFO] GPIO 35 (DT ) at rest: HIGH (pull-up OK)
[PASS] CLK and DT pull-ups confirmed — pins HIGH at rest
[INFO] GPIO 25 (SW ) at rest: HIGH (internal pull-up OK)
[PASS] SW pull-up confirmed — button reads HIGH when released
[INFO] Encoder interrupt attached on CLK (GPIO 34) RISING edge
```

**Interactive guided phases:**

**Phase 1 — Rotate CW 10 clicks:**
```
[PHASE 1] Rotate CW 10 clicks to validate CW detection

[EVENT] CW   Position: 1
[EVENT] CW   Position: 2
...
[EVENT] CW   Position: 10
[PASS] CW phase complete
```

**Phase 2 — Rotate CCW 10 clicks:**
```
[PHASE 2] Rotate CCW 10 clicks to validate CCW detection

[EVENT] CCW  Position: 9
[EVENT] CCW  Position: 8
...
[EVENT] CCW  Position: 0
[PASS] CCW phase complete
```

**Phase 3 — Press button 3 times:**
```
[PHASE 3] Press button 3 times to validate push-button

[EVENT] BUTTON PRESSED
[EVENT] BUTTON RELEASED  held=234ms
[INFO] Position reset to 0
...
[PASS] Button phase complete
```

**Summary:**
```
 VALIDATION SUMMARY
========================================
[STATE] Position: 0  CW: 10  CCW: 10  false: 0  button_presses: 3

[PASS] CW rotation detected (>= 10 steps)
[PASS] CCW rotation detected (>= 10 steps)
[PASS] Push-button press detected (>= 3 presses)
[PASS] False trigger count within tolerance (< 10)

 STATUS: PASS
========================================
[INFO] Counters reset — press reset or continue.
```

### Expected display output
None.

### Pass criteria
- GPIO 34 and GPIO 35 read HIGH at rest (external pull-ups confirmed)
- CW rotation: 10 CW events within first rotation phase
- CCW rotation: 10 CCW events within second rotation phase
- Button: 3 press+release events
- False triggers: < 10 across the entire test

### Fail criteria and troubleshooting

**Troubleshooting tree:**
```
GPIO 34 (CLK) reads LOW at rest?
  → External pull-up missing or wrong value
  → Check: 10kΩ from GPIO 34 to 3V3 rail
  → Without pull-up: encoder will count randomly when not turning

Rotation detected in wrong direction (CW shows CCW)?
  → Swap CLK and DT wires, OR
  → Invert DT logic in onClkRising(): change `if (dt)` to `if (!dt)`

Events fire but count is wrong (e.g., 5 events for 10 clicks)?
  → EC11 has 2 pulses per click (detent) — sketch counts pulses not detents
  → Divide by 2 to get click count. If CW=20 for 10 clicks, that's correct.

Excessive false triggers (> 10)?
  ├── Encoder cable too long — keep jumper wires short (< 20cm)
  ├── Cheap encoder with bouncy contacts — increase ENCODER_MIN_PULSE_US from 1000 to 5000
  └── Pull-up value too large (> 47kΩ) — use 10kΩ

Button doesn't register?
  → GPIO 25 wire missing or loose
  → SW common (C) not connected to GND
```

| Failure message | Cause | Fix |
|---|---|---|
| `CLK or DT not HIGH at rest` | Missing external pull-up | Add 10kΩ from GPIO 34/35 to 3V3 |
| Random counting when not rotating | Pull-up missing or GPIO floating | Add/check pull-ups |
| CW shows as CCW | CLK/DT swapped | Swap the two wires |
| Button shows 0 presses | SW to GND not connected | Check C/GND pin on encoder |

**Common mistakes:**
- Using `INPUT_PULLUP` in code on GPIO 34/35 — these pins do not support pull-up; the firmware uses `INPUT` (without pull-up) and the external resistors do the work
- Only one pull-up resistor (CLK has it, DT doesn't) — direction detection is wrong without DT pull-up
- Encoder module with its own VCC-powered pull-ups: VCC not connected → pins float despite "onboard pull-ups"

---

---

# Phase G — Analog Input

---

## Test 13 — Current Sensor (ADC)

**Hardware required:** ESP32, current sensor module (ACS712 5A/20A/30A or equivalent), USB cable, jumper wires
**Hardware optional:** Multimeter (verify sensor VCC), known load (to see non-zero current reading)
**Estimated time:** 10 minutes

### Wiring

```
ESP32 NodeMCU 38-pin          Current sensor module
─────────────                 ─────────────────────────
3V3  (pin 2)  ─────────────── VCC   (or 5V — check module datasheet)
GND  (pin 1)  ─────────────── GND
GPIO 36       ─────────────── OUT   (analog signal output)

IMPORTANT: GPIO 36 is INPUT-ONLY. Do NOT connect any active driver to it.
           Do NOT use INPUT_PULLUP (no internal pull-up available).
           The sensor output drives the pin; no additional resistors needed.
```

**ACS712 quiescent output voltage:** ~VCC/2 (≈1.65V on 3.3V supply, ≈2.5V on 5V supply). With no current flowing, the ADC reading should be near midscale.

### Required libraries
None — built-in ADC only.

### Arduino IDE / PlatformIO setup
- Open `13_current_sensor/13_current_sensor.ino`
- Upload → Serial Monitor at 115200
- No `#define` changes needed

### Expected serial output
```
========================================
 MMS V2 — Test 13: Current Sensor (ADC)
========================================
[INFO] GPIO 36 = ADC1 CH0 (SVP) — input-only
[INFO] ADC resolution: 12-bit (0–4095)
[INFO] ADC range: 0–3.3V (default 11dB attenuation)
[INFO] No processing applied — raw counts only

[INFO] === Phase 1: Single-shot read ===
[INFO] GPIO 36 raw ADC = 2048
[PASS] ADC reads non-saturated value

[INFO] === Phase 2: Continuous monitor (Ctrl+C or reset to stop) ===
[INFO] Expected: stable non-zero reading when sensor is powered

     #     raw    min   max  uptime_s
     ----  -----  ----  ----  --------
        1   2051  2051  2051         1
        2   2049  2049  2051         2
        3   2052  2049  2052         3
```

### Pass criteria
- Phase 1 ADC reading is between 100 and 3995 (not 0 or 4095, which indicate floating/shorted pin)
- Continuous monitor shows a stable value (variation < ±100 counts at zero current for ACS712)
- No compile errors or warnings

### Fail criteria and troubleshooting

| Failure message | Cause | Fix |
|---|---|---|
| `[WARN] ADC reads 0 or 4095` | Sensor not powered or OUT not connected | Check VCC on sensor; verify GPIO 36 → OUT wire |
| Reading is 0 constantly | GPIO 36 shorted to GND | Check wiring; GPIO 36 cannot be configured as output |
| Reading is 4095 constantly | OUT shorted to VCC or floating high | Check sensor VCC matches expected range; verify GND common |
| Very noisy reading (±200 counts) | Power supply noise | Add 100nF decoupling cap near sensor VCC–GND |

**Common mistakes:**
- Connecting sensor OUT to GPIO 34 or GPIO 35 instead of GPIO 36 (all three are input-only, but only 36 is the defined `PIN_CURRENT_SENSOR`)
- Powering ACS712 at 5V and connecting OUT directly to 3.3V GPIO: output swings 0–5V which exceeds GPIO input limit — use a voltage divider or level shifter, or power the sensor from 3.3V

---

---

# Execution Log Template

Print or keep open while validating. Check each box and record pass/fail.

```
MMS V2 Hardware Validation Log
Date: _____________  Engineer: ___________________
ESP32 Unit #: _______  Firmware: MMS_HW_VALIDATION

Test | Component              | Result | Notes
-----|------------------------|--------|--------------------------------
 01  | ESP32 System           | [ ] P/F |
 12  | LittleFS               | [ ] P/F |
 10  | WiFi                   | [ ] P/F | RSSI: ____ dBm
 11  | MQTT                   | [ ] P/F | Broker IP: ___________
 05  | OLED 128×32 I2C        | [ ] P/F |
 04  | OLED 128×64 SPI        | [ ] P/F |
 02  | MFRC522 RFID           | [ ] P/F | VersionReg: ____
 03  | HC89 Slot Sensor       | [ ] P/F | False triggers: ____
 06  | Relay Mechanical       | [ ] P/F | Active HIGH/LOW: ____
 07  | Relay SSR              | [ ] P/F |
 08  | WS2812 LED             | [ ] P/F | GRB/RGB: ____
 09  | Rotary Encoder EC11    | [ ] P/F | (Phase 4 — skip if N/A)
 13  | Current Sensor ADC     | [ ] P/F | Raw ADC reading: ____

All PASS? → Ready for integrated firmware flash.
Any FAIL? → Do not proceed. Resolve all failures before integration.

Sign-off: _______________________  Date: ___________
```

---

# Quick Reference: GPIO Assignments

| GPIO | Signal | Direction | Notes |
|---|---|---|---|
| 2 | Onboard LED | OUT | Active HIGH on most 38-pin boards |
| 4 | OLED 64 RST | OUT | SPI OLED reset |
| 5 | MFRC522 SS | OUT | SPI chip select |
| 16 | OLED 64 CS | OUT | SPI OLED chip select |
| 17 | OLED 64 DC | OUT | SPI data/command |
| 18 | SPI SCK | OUT | Shared: MFRC522 + OLED 64 |
| 19 | SPI MISO | IN | Shared: MFRC522 |
| 21 | I2C SDA | BIDIR | OLED 32 |
| 22 | I2C SCL | OUT | OLED 32 |
| 23 | SPI MOSI | OUT | Shared: MFRC522 + OLED 64 |
| 25 | Encoder SW | IN_PULLUP | Button, active LOW |
| 26 | Relay/SSR | OUT | Active HIGH default |
| 27 | MFRC522 RST | OUT | |
| 32 | HC89 | IN_PULLUP | Slot sensor, LOW=card present |
| 33 | WS2812 | OUT | Via 470Ω |
| 34 | Encoder CLK | IN | Input-only, external pull-up |
| 35 | Encoder DT | IN | Input-only, external pull-up |
| 36 | Current sensor | IN (analog) | ADC1 CH0 (SVP); input-only |

---

# Library Version Summary

| Library | Exact version tested | Install command |
|---|---|---|
| Adafruit SSD1306 | 2.5.9 | Library Manager |
| Adafruit GFX Library | 1.11.9 | Library Manager (auto-dep) |
| Adafruit BusIO | 1.16.1 | Library Manager (auto-dep) |
| MFRC522 by miguelbalboa | 1.4.11 | Library Manager |
| Adafruit NeoPixel | 1.12.3 | Library Manager |
| PubSubClient | 2.8.0 | Library Manager |
| ArduinoJson | 7.3.1 | Library Manager (select version 7.x) |
| ESP32 Arduino Core | 3.1.x | Boards Manager |
