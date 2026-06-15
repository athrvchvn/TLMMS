# Section 03 — Bill of Materials

← [System Architecture](02_system_architecture.md) | → [Machine Registry](04_machine_registry.md)

---

## Why This Section Exists

Every component listed here has a specific reason for existing and cannot be simply swapped for a "similar looking" part without risk. This section documents what to buy, why, and where.

**Critical notes before ordering:**
- The MFRC522 must be the **SPI version** (7-pin), not the I2C version
- OLEDs: one is **SPI 128×64** (7-pin), the other is **I2C 128×32** (4-pin) — they are different
- WS2812 must be **5V powered** — 3.3V will cause colour errors and insufficient brightness
- ESP32 must be **38-pin NodeMCU** variant — 30-pin or WROOM without breadboard headers will not fit the pin assignments in firmware

---

## BOM — Per Node

*Quantity for a 15-machine deployment assumes 15 nodes + 10% spare (17 total for critical parts).*

| # | Component | Purpose | Qty/Node | Qty (15 nodes) | Qty (+ spare) |
|---|---|---|---|---|---|
| 1 | ESP32 NodeMCU 38-pin | Main controller | 1 | 15 | 17 |
| 2 | MFRC522 RFID module (SPI) | Card reading | 1 | 15 | 17 |
| 3 | HC89 Optical slot sensor | Card presence detection | 1 | 15 | 17 |
| 4 | SSD1306 128×64 OLED — SPI (7-pin) | Primary display | 1 | 15 | 17 |
| 5 | SSD1306 128×32 OLED — I2C (4-pin) | Status strip display | 1 | 15 | 17 |
| 6 | Single-channel 5V relay module | Machine power control (inductive loads) | 1 | 15 | 17 |
| 7 | Solid State Relay (SSR) 25A | Machine power control (resistive/heating) | 0–1 | 0–15 | As needed |
| 8 | WS2812B LED (1 pixel or ring) | Visual state indicator | 1 | 15 | 20 |
| 9 | HiLink HLK-PM01 (5V 3W) | 240V AC → 5V DC PSU | 1 | 15 | 17 |
| 10 | Rotary Encoder EC11 | PIN entry (Phase 4 only) | 0 | 0 | 0 |
| 11 | 470Ω resistor (1/4W) | WS2812 data line protection | 1 | 15 | 30 |
| 12 | 10kΩ resistor (1/4W) | Encoder CLK/DT pull-ups (Phase 4) | 0 | 0 | 0 |
| 13 | 100µF 10V electrolytic cap | WS2812 VCC decoupling | 1 | 15 | 20 |
| 14 | 10µF 10V electrolytic cap | General decoupling on 3.3V rail | 2 | 30 | 40 |
| 15 | Project enclosure (ABS, ~160×120×60mm) | Weatherproof housing | 1 | 15 | 16 |
| 16 | PCB / Perfboard (70×50mm min) | Component mounting | 1 | 15 | 17 |
| 17 | 2.54mm pin headers (male, 40-pin strip) | ESP32 socket | 2 strips | 30 | 35 |
| 18 | 2.54mm female header sockets | ESP32 socketing | 2 strips | 30 | 35 |
| 19 | JST-XH 2.54mm 2-pin connector pair | Relay output terminals | 2 | 30 | 35 |
| 20 | JST-PH 2.0mm 4-pin connector (or screw terminal) | RFID module connection | 1 | 15 | 17 |
| 21 | 3.5mm 2-pin screw terminal block | AC input / machine power output | 2 | 30 | 35 |
| 22 | 5A inline fuse + holder | AC protection | 1 | 15 | 20 |
| 23 | 22AWG silicone hookup wire (red/black/other, 5m) | Internal wiring | 5m | 75m | 90m |
| 24 | Velcro or M3 standoffs | ESP32 mounting inside enclosure | 4 | 60 | 70 |
| 25 | M3×10 screws + nuts | Enclosure assembly | 8 | 120 | 140 |
| 26 | Cable gland PG9 (or similar) | AC cable entry weatherproofing | 2 | 30 | 35 |
| 27 | Heat shrink tubing assorted | Wire insulation | — | 1 pack | 1 pack |
| 28 | Multimeter | Testing (shared tool) | — | 1 | 1 |

---

## Infrastructure Components (Lab-Wide, Not Per Node)

| # | Component | Purpose | Qty | Notes |
|---|---|---|---|---|
| I1 | Raspberry Pi 4 Model B (2GB) | MQTT broker + Firebase bridge | 1 | RPi 3B+ also works |
| I2 | RPi official power supply (5.1V 3A USB-C) | Reliable RPi power | 1 | Do not use phone charger |
| I3 | MicroSD card 32GB Class 10 | RPi OS + logs | 1 | Samsung or SanDisk Pro |
| I4 | Ethernet cable Cat5e | RPi to router (static IP) | 1m | WiFi on RPi is unreliable for a broker |
| I5 | Network router with DHCP + static IP reservation | Lab LAN | 1 | Existing lab router |
| I6 | 2.4 GHz WiFi AP | ESP32 connectivity | 1 | Existing lab AP; ensure 2.4 GHz band enabled |

---

## Card Issuing Station (Kiosk) Components

| # | Component | Purpose | Qty |
|---|---|---|---|
| K1 | Laptop or Raspberry Pi | Runs Flask kiosk app | 1 |
| K2 | MFRC522 RFID module (same as node) | Reads and writes student cards | 1 |
| K3 | USB-to-serial FTDI/CP2102 + Arduino Nano (or ESP32) | Bridge between Python and MFRC522 | 1 |
| K4 | MIFARE Classic 1K cards (white, printable) | Student cards | 400 (for 300 students + spares) |

---

## Detailed Component Notes

### ESP32 NodeMCU 38-pin

**Why this exact variant:** The 38-pin form factor provides access to GPIO 34/35 (input-only pins used for encoder), GPIO 25/26/27, and 3V3/GND rails on both sides of the board. 30-pin variants lack these.

**Supplier:** Robocraze, Robu.in, Amazon India
**Alternative:** ESP32 DevKit V1 38-pin (different silkscreen, identical pinout)
**Estimated cost:** ₹350–400 each

**What to check when receiving:**
- Count pins: 19 per side = 38 total
- Confirm USB-to-serial chip (CP2102 or CH340) is present
- Upload a blink sketch before assembly to confirm functionality

---

### MFRC522 RFID Module

**Why SPI version:** The firmware uses the Arduino MFRC522 library which requires SPI. I2C versions use a different library and the pin assignments in `config.h` are for SPI.

**Critical:** Run at 3.3V — **NOT 5V**. Connecting VCC to 5V may work temporarily but can damage the chip.

**Connector labels on module:** VCC, GND, RST, IRQ (unused), MISO, MOSI, SCK, SDA (= SS/CS)

**Supplier:** Robocraze, Amazon India
**Estimated cost:** ₹180–250 each

**What to check:**
- Run Test 02 from hardware validation suite — `VersionReg` should read `0x91` or `0x92`
- Many cheap clones return wrong VersionReg but still function — confirm with actual card tap

---

### HC89 Optical Slot Sensor

**Why not a direct RFID tap?** A slot sensor requires the card to be physically held in the slot. This is the only way to cleanly detect "card removed" (end of session). Without it, the system cannot know when the student has finished.

**Output logic:** OUT = LOW when card in slot (beam blocked), HIGH when empty. Firmware uses `INPUT_PULLUP` on GPIO 32.

**Supplier:** Robu.in, AliExpress
**Alternative:** Any IR slotted optical sensor with TTL output compatible with 3.3V
**Estimated cost:** ₹80–150 each

---

### SSD1306 128×64 OLED (SPI, 7-pin)

**Why 7-pin SPI version:** The firmware uses Adafruit SSD1306 SPI constructor with DC, CS, and RST pins. The I2C 4-pin version uses a different constructor and different GPIO pins.

**Pin labels (7-pin SPI):** GND, VCC, D0(CLK), D1(MOSI), RES(RST), DC, CS

**Supplier:** Robocraze, Amazon India
**Estimated cost:** ₹250–350 each

---

### SSD1306 128×32 OLED (I2C, 4-pin)

**Why 128×32?** This display is used as an always-visible status strip at the bottom of the enclosure showing WiFi/MQTT status, machine name, and time. The narrower height (32px vs 64px) fits a horizontal strip layout.

**Pin labels:** GND, VCC, SCL, SDA
**I2C address:** 0x3C (default; 0x3D if address jumper is bridged)

**Supplier:** Robocraze, Amazon India
**Estimated cost:** ₹200–280 each

---

### Mechanical Relay Module (5V single-channel)

**When to use:** For machines with inductive loads (motors, solenoids) or where switching clicks are acceptable (soldering stations, grinder). The relay is galvanically isolated — the 5V control circuit is completely separate from the mains circuit.

**Relay polarity:** Most optocoupler-based blue relay modules are active HIGH (IN HIGH = relay ON). Some bare relay modules are active LOW. The firmware reads `relay_active_high` from NVS; this is set via MQTT config push.

**Safety:** ALWAYS ensure relay contacts are rated higher than the machine's actual load (current × voltage). Use a 10A relay for a 5A machine as minimum safety margin.

**Supplier:** Robocraze, Amazon India
**Estimated cost:** ₹120–180 each

---

### Solid State Relay (SSR)

**When to use:** For purely resistive heating loads (hot air guns, soldering irons with no motor), where silent operation is required, or where the machine is switched very frequently.

**SSR type:** Use DC-controlled AC output SSR (e.g., Fotek SSR-25 DA). Control input: 3–32V DC (GPIO 3.3V drives directly — no extra components needed). Load output: 24–380V AC.

**Known limitation:** SSRs have off-state leakage (1–3V on load terminals). This is normal and does not mean the SSR is stuck on. Do not use SSR for machines sensitive to leakage current.

**Estimated cost:** ₹350–600 each

---

### HiLink HLK-PM01 (5V 3W)

**Why this PSU:** The HiLink series is a certified, compact PCB-mount AC-to-DC converter. It fits inside the enclosure and eliminates the need for an external wall adapter. Output: 5V 600mA (3W).

**Power budget per node:**
- ESP32: ~250mA peak (WiFi active)
- OLED 64 + 32: ~25mA each
- WS2812: ~60mA peak
- MFRC522: ~50mA
- Total: ~410mA peak → HLK-PM01 at 600mA provides 190mA headroom

**Safety:** The HiLink module handles AC mains. Enclose it in a non-conductive housing. Do not touch while mains is connected. Add a 5A fuse on the AC input.

**Supplier:** Robu.in, Amazon India
**Estimated cost:** ₹280–400 each

---

### WS2812B LED

**5V power is mandatory.** 3.3V operation causes dim, colour-inaccurate LEDs. The 100µF capacitor across VCC-GND prevents the initial power surge from damaging the IC. The 470Ω series resistor on the data line suppresses ringing on long wires.

**Byte order:** Default is `NEO_GRB`. If colours appear wrong (red = green), change to `NEO_RGB` in `led_controller.cpp`.

**Supplier:** Robocraze, Amazon India
**Estimated cost:** ₹50–120 per LED/ring

---

## Cost Estimate Summary (15 Nodes)

| Category | Estimated Cost (INR) |
|---|---|
| ESP32 × 17 | ₹6,300 |
| MFRC522 × 17 | ₹3,800 |
| HC89 × 17 | ₹2,200 |
| OLED 64 SPI × 17 | ₹5,100 |
| OLED 32 I2C × 17 | ₹4,300 |
| Relay modules × 17 | ₹2,600 |
| HiLink PSU × 17 | ₹5,800 |
| WS2812 × 20 | ₹1,600 |
| Enclosures × 15 | ₹6,000 |
| Passives (resistors, caps, wire, connectors) | ₹3,000 |
| Raspberry Pi 4 + accessories | ₹4,500 |
| MIFARE cards × 400 | ₹5,000 |
| Kiosk hardware | ₹2,000 |
| **Total estimate** | **~₹52,200** |

*Prices as of 2024. Add 15% for shipping/GST. Add 10% contingency buffer.*
