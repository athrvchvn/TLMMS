# Section 05 — Hardware Assembly Guide

← [Machine Registry](04_machine_registry.md) | → [Hardware Validation](06_hardware_validation.md)

---

## Before You Start

**Read the machine registry first** ([Section 04](04_machine_registry.md)). Know which machine ID this node is for before wiring anything. The relay type (mechanical vs SSR) depends on the machine.

**Safety rules:**
- Never connect AC mains while any signal wires are being worked on
- Test all 5V/3.3V DC wiring with a multimeter before first power-on
- The HiLink PSU handles 240V AC — treat it as a live shock hazard at all times
- Use a fuse on the AC input before the HiLink

**Tools required:**
- Soldering iron + solder
- Wire stripper
- Multimeter (DC voltage + continuity modes)
- Small flathead screwdriver (for screw terminals)
- USB cable for programming (keep handy for flashing)

---

## Complete Pin Mapping Table

| Signal | ESP32 GPIO | Direction | Pull resistor | Notes |
|---|---|---|---|---|
| MFRC522 SS | GPIO 5 | OUT | None | SPI chip select |
| MFRC522 RST | GPIO 27 | OUT | None | Active LOW reset |
| SPI SCK | GPIO 18 | OUT | None | Shared: RFID + OLED64 |
| SPI MISO | GPIO 19 | IN | None | Shared: RFID + OLED64 |
| SPI MOSI | GPIO 23 | OUT | None | Shared: RFID + OLED64 |
| OLED64 DC | GPIO 17 | OUT | None | Data/Command select |
| OLED64 CS | GPIO 16 | OUT | None | SPI chip select |
| OLED64 RST | GPIO 4 | OUT | None | Active LOW reset |
| OLED32 SDA | GPIO 21 | BIDIR | Built-in (device) | I2C data |
| OLED32 SCL | GPIO 22 | OUT | Built-in (device) | I2C clock |
| Relay IN | GPIO 26 | OUT | None | Firmware drives LOW before pinMode |
| HC89 OUT | GPIO 32 | IN | INPUT_PULLUP | LOW = card present |
| WS2812 DIN | GPIO 33 | OUT | None | Via 470Ω series resistor |
| Current sensor OUT | GPIO 36 | IN (analog) | None | ADC1 CH0 (SVP) — **input-only, do not drive** |
| Encoder CLK | GPIO 34 | IN | EXT 10kΩ to 3V3 | Input-only; Phase 4 |
| Encoder DT | GPIO 35 | IN | EXT 10kΩ to 3V3 | Input-only; Phase 4 |
| Encoder SW | GPIO 25 | IN | INPUT_PULLUP | Phase 4 |
| WiFi status LED | GPIO 2 | OUT | None | Onboard LED |

---

## Power Distribution

```
AC Mains (240V) ─── [Fuse 5A] ─── HiLink HLK-PM01
                                         │
                              ┌──────────┴──────────┐
                              │       5V DC          │
                              │                      │
                    ┌─────────┴──────┐    ┌──────────┴────────┐
                    │  ESP32 Vin pin  │    │  WS2812 VCC       │
                    │  (5V → 3.3V    │    │  (5V directly)    │
                    │   internal LDO)│    └───────────────────┘
                    └─────────┬──────┘
                              │  3.3V (from ESP32 3V3 pin)
                    ┌─────────┴──────────────────────────┐
                    │  MFRC522 VCC                       │
                    │  OLED64 VCC                        │
                    │  OLED32 VCC                        │
                    │  HC89 VCC                          │
                    └────────────────────────────────────┘

Relay module: 5V coil → Vin pin
              Signal: GPIO 26 (3.3V logic, relay module has optocoupler)

SSR (if used): Control INPUT+ → GPIO 26 (3.3V drives directly)
               Control INPUT- → GND
               Load terminals: in series with machine AC circuit
```

---

## Full Node Wiring Diagram

```
                          ESP32 NodeMCU 38-pin
                    ┌─────────────────────────────┐
                    │ 3V3  ○─────────────────────  │──── MFRC522 3.3V
                    │                             │──── OLED64 VCC
                    │                             │──── OLED32 VCC
                    │                             │──── HC89 VCC
                    │ GND  ○─────────────────────  │──── All GND rails
                    │ Vin  ○─────────────────────  │──── HiLink 5V+ output
                    │                             │──── WS2812 VCC
                    │                             │──── Relay VCC
                    │ GPIO 5  ○──────────────────  │──── MFRC522 SDA (SS)
                    │ GPIO 18 ○──────────────────  │──── MFRC522 SCK ─────────── OLED64 D0
                    │ GPIO 19 ○──────────────────  │──── MFRC522 MISO
                    │ GPIO 23 ○──────────────────  │──── MFRC522 MOSI ────────── OLED64 D1
                    │ GPIO 27 ○──────────────────  │──── MFRC522 RST
                    │ GPIO 17 ○──────────────────  │──── OLED64 DC
                    │ GPIO 16 ○──────────────────  │──── OLED64 CS
                    │ GPIO 4  ○──────────────────  │──── OLED64 RES
                    │ GPIO 21 ○──────────────────  │──── OLED32 SDA
                    │ GPIO 22 ○──────────────────  │──── OLED32 SCL
                    │ GPIO 26 ○──────────────────  │──── Relay IN
                    │ GPIO 32 ○──────────────────  │──── HC89 OUT
                    │ GPIO 33 ○──[470Ω]──────────  │──── WS2812 DIN
                    └─────────────────────────────┘

WS2812B LED:
  VCC → [100µF cap+] → Vin (5V)
  GND → [100µF cap-] → GND
  DIN ← 470Ω ← GPIO 33

Relay output (load circuit):
  Machine AC live wire ─── COM
  Machine power input  ─── NO (normally open)
  (When relay energised, COM connects to NO → machine receives power)

OR SSR:
  AC live ─── SSR LOAD1
  Machine ─── SSR LOAD2
  GPIO26  ─── SSR INPUT+
  GND     ─── SSR INPUT-
```

---

## MFRC522 Connection Detail

```
MFRC522 module (8 pins from left):
  Pin 1: SDA  ──── GPIO 5  (SPI chip select, labelled SS in firmware)
  Pin 2: SCK  ──── GPIO 18
  Pin 3: MOSI ──── GPIO 23
  Pin 4: MISO ──── GPIO 19
  Pin 5: IRQ  ──── NC (not connected)
  Pin 6: GND  ──── GND
  Pin 7: RST  ──── GPIO 27
  Pin 8: 3.3V ──── 3V3 (NOT 5V)
```

**Common mistake:** Many MFRC522 modules label the SPI CS pin as "SDA". This is confusing — it is not the I2C SDA pin. In the firmware it is configured as the SPI Slave Select.

---

## SPI OLED 128×64 Connection Detail

```
OLED 64 module (7 pins):
  Pin 1: GND ──── GND
  Pin 2: VCC ──── 3V3
  Pin 3: D0  ──── GPIO 18 (shared SCK with MFRC522)
  Pin 4: D1  ──── GPIO 23 (shared MOSI with MFRC522)
  Pin 5: RES ──── GPIO 4
  Pin 6: DC  ──── GPIO 17
  Pin 7: CS  ──── GPIO 16
```

**The SPI bus is shared between MFRC522 and the OLED.** This works because they have different CS/SS pins (GPIO 5 for RFID, GPIO 16 for OLED). The firmware drives these CS pins correctly — only one device sees the SPI bus at any time.

---

## I2C OLED 128×32 Connection Detail

```
OLED 32 module (4 pins):
  GND ──── GND
  VCC ──── 3V3
  SCL ──── GPIO 22
  SDA ──── GPIO 21
```

No additional pull-up resistors needed — the SSD1306 module has onboard 4.7kΩ pull-ups on SDA and SCL.

---

## Relay Module Connection Detail

```
Relay module (single channel, 5V):
  VCC  ──── Vin (5V)
  GND  ──── GND
  IN   ──── GPIO 26

Load connections (screw terminals):
  COM  ──── AC live wire from mains (via HiLink fused input)
  NO   ──── Machine power input (live wire to machine)
  NC   ──── (Not used in normal operation)
```

**Why GPIO 26 is driven LOW before `pinMode(OUTPUT)`:** The ESP32 GPIO boot state is unpredictable. By writing LOW before setting the pin as output, we guarantee the relay does not accidentally energise on power-up (which could start the machine unexpectedly). See `relay_controller.cpp` line 1: `digitalWrite(PIN_RELAY, LOW); pinMode(PIN_RELAY, OUTPUT);`

---

## HC89 Slot Sensor Connection Detail

```
HC89 (3 or 4 pins):
  VCC  ──── 3V3 (or 5V if module spec requires — check datasheet)
  GND  ──── GND
  OUT  ──── GPIO 32

Note: GPIO 32 is configured as INPUT_PULLUP.
Most HC89 modules have open-collector output and rely on this pull-up.
```

**Orientation in enclosure:** The slot opening should face outward from the enclosure so cards can be inserted. The slot width is typically 2–3mm — ensure the MIFARE card fits without force.

---

## WS2812B LED Connection Detail

```
WS2812B (strip cut to 1 LED, or ring module):
  VCC ──── [100µF electrolytic, + to VCC] ──── Vin (5V)
  GND ──── GND (also cap– to GND)
  DIN ──── [470Ω resistor] ──── GPIO 33

The 100µF capacitor is placed ACROSS the VCC–GND pins AT THE LED.
Not at the ESP32 — at the LED module itself, as close as possible.
```

**LED placement:** Mount the WS2812 so it is visible from outside the enclosure. A small hole or light-diffusing acrylic window works well.

---

## Enclosure Layout

```
Front panel (facing operator):
┌─────────────────────────────────────────────────┐
│                                                 │
│   ┌─────────────────────────────────────┐       │
│   │       128×64 SPI OLED               │       │
│   │   (primary state display)           │       │
│   └─────────────────────────────────────┘       │
│                                                 │
│        [WS2812 RGB LED indicator]               │
│                                                 │
│   ┌─────────────────────────────────────┐       │
│   │  128×32 I2C OLED (status strip)     │       │
│   └─────────────────────────────────────┘       │
│                                                 │
│   ┌──────────────────┐                          │
│   │   HC89 SLOT      │ ← card inserted here     │
│   └──────────────────┘                          │
│                                                 │
└─────────────────────────────────────────────────┘

Internal layout:
┌─────────────────────────────────────────────────┐
│  [HiLink PSU]  [Relay/SSR]  [Fuse holder]       │
│                                                 │
│  [ESP32 on perfboard/PCB]                       │
│    ├── MFRC522 (mounted behind HC89 slot)        │
│    ├── OLED 64 (front panel)                    │
│    └── OLED 32 (front panel, below OLED 64)     │
│                                                 │
│  [Cable gland: AC in]  [Cable gland: machine]   │
└─────────────────────────────────────────────────┘
```

---

## Assembly Checklist

Perform this checklist for every node before first power-on.

- [ ] All GND rails connected to common GND
- [ ] MFRC522 VCC = 3.3V (NOT 5V) — confirmed with multimeter
- [ ] ESP32 Vin = 5V from HiLink — confirmed with multimeter
- [ ] WS2812 VCC = 5V — confirmed with multimeter
- [ ] 100µF capacitor across WS2812 VCC–GND (observe polarity: + to VCC)
- [ ] 470Ω resistor in series on WS2812 DIN
- [ ] Relay IN connected to GPIO 26 (not GPIO 16 or any SPI pin)
- [ ] HC89 OUT connected to GPIO 32
- [ ] OLED64: DC=17, CS=16, RST=4 — verified against pin table
- [ ] OLED32: SDA=21, SCL=22 — verified against pin table
- [ ] MFRC522: SS=5, RST=27 — verified against pin table
- [ ] AC fuse installed and rated correctly (5A for a 600mA load)
- [ ] AC cable glands tight (no bare wire visible outside enclosure)
- [ ] ESP32 USB port accessible for firmware flashing without disassembly
- [ ] Physical card can be inserted and removed from HC89 slot freely
- [ ] Relay output terminals clearly labelled (COM, NO) on enclosure
- [ ] Current sensor OUT connected to GPIO 36 (input-only — do not apply digital drive to this pin)

**Hardware validation must pass before proceeding to node provisioning.** See [Section 06](06_hardware_validation.md).
