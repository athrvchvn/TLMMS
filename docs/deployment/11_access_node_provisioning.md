# Section 11 — Access Node Provisioning

← [Card Issuing Station](10_card_issuing_station.md) | → [Integration Testing](12_integration_testing.md)

---

## What Provisioning Means

"Provisioning" means configuring a validated ESP32 node with its unique identity and secrets, then flashing the production firmware. After provisioning, the node:

- Knows its machine ID (which machine it controls)
- Knows the lab's master key (for RFID sector key derivation)
- Can connect to WiFi and the MQTT broker
- Will receive its full config (session limit, relay type, machine name) from the bridge on first connection

Provisioning is done once per node. If the master key ever changes, all nodes must be re-provisioned.

---

## Prerequisites

Before provisioning any node:
- [ ] Hardware validation passed (Section 06)
- [ ] RPi is running Mosquitto and bridge.py (Section 07)
- [ ] Firebase is set up and machines collection is seeded (Section 08)
- [ ] Master key has been generated (Section 10, Step 1)
- [ ] You know which machine ID this node corresponds to (Section 04)

---

## Step 1 — Prepare secrets.h

In the Arduino IDE, the `secrets.h` file is opened separately per node. Each node gets the same master key but a unique `MACHINE_ID`.

```bash
# From repo root:
cp firmware/access_node/secrets.h.template firmware/access_node/secrets.h
```

Edit `firmware/access_node/secrets.h`:
```cpp
// WiFi
#define WIFI_SSID     "tlmms"
#define WIFI_PASSWORD "your_wifi_password_here"

// MQTT
#define MQTT_BROKER   "192.168.0.10"   // RPi IP — must match static IP from Section 07
#define MQTT_PORT     1883

// Master key — 32 bytes, same for ALL nodes in the lab
static const uint8_t MASTER_KEY[32] = {
    0xA1, 0x3F, 0x88, ...,   // fill in from master key generation
};
```

**Do not commit this file.** It is in `.gitignore`. Do not create it until you are at the node and ready to flash.

---

## Step 2 — Set Machine ID in config.h

```bash
# Edit firmware/access_node/config.h
```

Find and update:
```cpp
#define MACHINE_ID    1          // ← Change this for each node (1–5)
#define MACHINE_NAME  "Creality 3D Printer"  // ← Change to match
```

**Canonical machine table — use exactly these values:**

| MACHINE_ID | MACHINE_NAME |
|---|---|
| 1 | Creality 3D Printer |
| 2 | Fracktal Laser Cutter |
| 3 | Soldering Station 1 |
| 4 | Soldering Station 2 |
| 5 | Bosche Grinder |

**Important:** The name here is only a fallback. After first MQTT connect, the bridge pushes the name from Firestore and the node updates NVS. But until that first connection, this is what shows on the OLED.

---

## Step 3 — Build and Flash

In Arduino IDE:
1. Open `firmware/access_node/access_node.ino`
2. Verify board settings:
   - Board: `ESP32 Dev Module`
   - Partition Scheme: **`Huge APP (3MB No OTA/1MB SPIFFS)`** ← production firmware; use the OTA partition scheme if OTA is needed: `Default 4MB with spiffs`
   - Upload Speed: `921600`
   - CPU Frequency: `240MHz`
3. Click **Verify** first (Ctrl+R) — confirm no compile errors
4. Connect USB cable to the node
5. Click **Upload** (Ctrl+U)

Expected output at the end of upload:
```
Leaving...
Hard resetting via RTS pin...
```

---

## Step 4 — Open Serial Monitor and Verify Boot

Baud rate: **115200**

Expected output within 5 seconds:
```
MMS V2 Access Node
Machine ID: 1 (Creality 3D Printer)
FW: 2.0.0

[HW] Relay init — GPIO 26 LOW before OUTPUT
[HW] MFRC522 SPI init OK
[HW] OLED 128x64 SPI init OK
[HW] OLED 128x32 I2C init OK
[HW] HC89 slot sensor init — GPIO 32 INPUT_PULLUP
[HW] LittleFS mounted OK (used 0 bytes / 196608 bytes)
[HW] WS2812 init OK

[NET] Connecting to WiFi tlmms ...
[NET] WiFi connected — IP: 192.168.0.101, RSSI: -52 dBm
[NET] NTP synced — 14:32:11 IST

[MQTT] Connecting to 192.168.0.10:1883 ...
[MQTT] Connected — client ID: mms_node_1
[MQTT] Subscribed: mms/nodes/1/command
[MQTT] Subscribed: mms/nodes/1/config
[MQTT] Subscribed: mms/all/revoked
[MQTT] Published status: {state:"idle", fw:"2.0.0", ip:"192.168.0.101"}
[MQTT] Received config: {name:"Creality 3D Printer", session_limit:60, relay_active_high:1, machine_active:1}
[NVS] Config saved to NVS

[SYS] Watchdog started — 30s timeout
[SYS] IDLE — tap card to begin
```

### If WiFi fails:
```
[NET] WiFi failed after 10s — proceeding offline
```
The node will still work for access control. Sessions are logged to LittleFS and flushed when WiFi/MQTT reconnects.

### If MQTT fails:
```
[MQTT] Connection failed (rc=-2) — will retry in 5s
```
Retry is automatic. Node still operates offline.

---

## Step 5 — Verify OLED Displays

Check the physical node:

**128×64 OLED (primary):**
```
┌─────────────────────┐
│   Tinkerers' Lab    │
│     IIT Indore      │
│                     │
│  Creality 3D Printer│
│                     │
│  Tap card to start  │
└─────────────────────┘
```

**128×32 OLED (status strip):**
```
[WiFi●] [MQTT●]  CREALITY 3D PR  14:32
```
WiFi and MQTT dots should be filled (connected).

**WS2812 LED:**
Should be slowly breathing white (IDLE state).

---

## Step 6 — Verify Card Access

Use a card that was issued via the kiosk (Section 10) with permission bit 0 set (machine ID 1).

1. Insert the card into the HC89 slot
2. Watch serial output:
```
[HC89] Card present
[RFID] UID: A1B2C3D4
[RFID] Deriving sector key for UID: A1B2C3D4
[RFID] Auth Sector 1 OK
[RFID] Block 4 read: roll=220002123, schema=0x02
[RFID] Block 5 read: pin_hash=aabbccdd..., perm_mask=0x0000001
[ACCESS] Revocation check — UID A1B2C3D4 not revoked
[ACCESS] Permission check — bit 0 = 1 — GRANTED
[RELAY] ON (active HIGH)
[SESSION] Started: roll=220002123, machine=1, limit=60min
[MQTT] Published status: {state:"active", roll:"220002123"}
```

3. Remove the card:
```
[HC89] Card absent
[RELAY] OFF
[SESSION] Ended: duration=45s, reason=card_removed
[MQTT] Published event: {ts:..., roll:"220002123", m:1, e:"session_end", d:45, er:"card_removed"}
[FS] Log appended: sessions.jsonl
```

4. Check Firebase Console → Firestore → sessions → confirm a new document appeared

---

## Step 7 — Verify Denial for Wrong Machine

Use the same card on a node where that card does NOT have permission (e.g., if machine_permissions=1, test on machine ID 2):
```
[ACCESS] Permission check — bit 1 = 0 — DENIED
[RELAY] stays OFF
```
OLED should show "Access Denied" and the reason.

---

## Step 8 — Verify Offline Operation

1. On the RPi, stop the bridge: `sudo systemctl stop mms-bridge`
2. On the node, observe serial — MQTT will show "Connection failed" retries
3. Insert a permitted card
4. Access should still be granted (offline mode)
5. Session is logged to LittleFS
6. Restart bridge: `sudo systemctl start mms-bridge`
7. Watch serial — node reconnects to MQTT
8. Buffered log is flushed:
```
[FS] Flushing 1 events from LittleFS
[MQTT] Published event (from cache): ...
[FS] Log cleared after flush
```
9. Check Firestore — the offline session should now appear

---

## Provisioning Checklist

Photocopy this checklist for each node.

```
Node Provisioning Sign-off
Machine ID:  [ ]  Name: ________________________
Date: __________  Engineer: ____________________

Pre-flash:
[ ] secrets.h filled in (WiFi, broker IP, master key)
[ ] config.h: MACHINE_ID and MACHINE_NAME match machine registry
[ ] Partition scheme: Default 4MB with spiffs
[ ] Compile successful — 0 errors

Post-flash:
[ ] Serial boot log shows hardware init OK (all [HW] lines pass)
[ ] WiFi connected — IP assigned
[ ] NTP synced — IST time shown on OLED32
[ ] MQTT connected — client ID: mms_node_[N]
[ ] Config received from bridge — NVS updated
[ ] IDLE state on OLED64
[ ] WS2812: slow white breathing

Access tests:
[ ] Permitted card → GRANTED, relay ON, OLED shows roll+timer
[ ] Remove card → relay OFF, session in Firestore
[ ] Non-permitted card → DENIED, relay stays OFF
[ ] Offline test → access still granted, session in LittleFS
[ ] Reconnect → LittleFS events flushed to Firestore

Sign-off: _______________________________
```

---

## Common Mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| Wrong `MACHINE_ID` | Sessions logged under wrong machine | Re-flash with correct ID |
| `MQTT_BROKER` wrong IP | Node retries forever, never connects | Check RPi static IP; confirm reachable with `ping` from laptop |
| Partition scheme wrong | Firmware too large to flash | Select "Default 4MB with spiffs" |
| `secrets.h` not filled in | Compile error or all-zero master key | Fill in all 32 bytes of MASTER_KEY |
| Different master key than kiosk | Sector auth fails on every card | Ensure all nodes and kiosk use identical MASTER_KEY bytes |
| Flashing before assembly checklist | Hardware init failures misidentified as firmware bugs | Always complete hardware validation first |
| Config not received (no bridge) | NVS uses fallback values from config.h | Start bridge.py before provisioning |
