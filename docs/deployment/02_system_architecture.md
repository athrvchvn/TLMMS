# Section 02 — System Architecture

← [Executive Overview](01_executive_overview.md) | → [Bill of Materials](03_bill_of_materials.md)

---

## Overall Architecture

```
╔══════════════════════════════════════════════════════════════════════╗
║                        LAB NETWORK (2.4 GHz WiFi)                    ║
║                                                                      ║
║  ┌──────────┐  ┌──────────┐  ┌──────────┐     ┌──────────┐           ║
║  │ Node 1   │  │ Node 2   │  │ Node 3   │ ... │ Node 15  │           ║
║  │Creality  │  │ Laser    │  │Soldering │     │ Grinder  │           ║
║  └────┬─────┘  └────┬─────┘  └────┬─────┘     └────┬─────┘           ║
║       │              │              │                │               ║
║       └──────────────┴──────────────┴────────────────┘               ║
║                                 │ MQTT port 1883                     ║
║                                 ▼                                    ║
║              ┌─────────────────────────────────────┐                 ║
║              │  Raspberry Pi (192.168.0.10 static) │                 ║
║              │  ├── Mosquitto MQTT broker          │                 ║
║              │  └── bridge.py (systemd service)    │                 ║
║              └───────────────────┬─────────────────┘                 ║
╚══════════════════════════════════╪═══════════════════════════════════╝
                                   │ HTTPS / Firebase Admin SDK
                                   ▼
            ╔══════════════════════════════════════════╗
            ║         FIREBASE (Google Cloud)           ║
            ║  ┌──────────────────────────────────┐    ║
            ║  │ Firestore                         │    ║
            ║  │  /users /machines /sessions       │    ║
            ║  │  /revoked /admins /super_admins   │    ║
            ║  └──────────────────────────────────┘    ║
            ║  ┌──────────────────────────────────┐    ║
            ║  │ Realtime Database (RTDB)          │    ║
            ║  │  /mms/nodes /mms/commands         │    ║
            ║  │  /mms/ota   /mms/revoked          │    ║
            ║  └──────────────────────────────────┘    ║
            ║  ┌──────────────────────────────────┐    ║
            ║  │ Storage (OTA firmware binaries)   │    ║
            ║  └──────────────────────────────────┘    ║
            ╚══════════════════════╦═══════════════════╝
                                   ║ Firebase JS SDK
                                   ▼
            ╔══════════════════════════════════════════╗
            ║     REACT WEB APP (Firebase Hosting)     ║
            ║  Admin dashboard + super admin panel     ║
            ╚══════════════════════════════════════════╝
```

---

## Access Node Architecture

Each physical machine has one access node. The node is self-contained — it can grant or deny access even with no network connection.

```
                     ┌────────────────────────────────────────┐
                     │           ESP32 NodeMCU 38-pin          │
                     │                                        │
  HC89 Slot          │ GPIO 32 ◄── Card present/absent        │
  Sensor ────────────┤                                        │
                     │ GPIO 5  ◄── MFRC522 SS (SPI)          │
  MFRC522            │ GPIO 27 ◄── MFRC522 RST               │
  RFID ──────────────┤ GPIO 18 ◄── SPI SCK (shared)          │
                     │ GPIO 19 ◄── SPI MISO (shared)         │
                     │ GPIO 23 ◄── SPI MOSI (shared)         │
                     │                                        │
  128×64 OLED        │ GPIO 17 ──► OLED DC                   │
  SPI ───────────────┤ GPIO 16 ──► OLED CS                   │
                     │ GPIO 4  ──► OLED RST                   │
                     │ GPIO 18/23 shared with RFID            │
                     │                                        │
  128×32 OLED        │ GPIO 21 ◄► I2C SDA                    │
  I2C ───────────────┤ GPIO 22 ──► I2C SCL                   │
                     │                                        │
  Relay / SSR ───────┤ GPIO 26 ──► Relay IN                  │
                     │                                        │
  WS2812 LED ────────┤ GPIO 33 ──► LED DIN (via 470Ω)        │
                     │                                        │
  Rotary Encoder     │ GPIO 34 ◄── CLK (ext. pull-up)        │
  (Phase 4) ─────────┤ GPIO 35 ◄── DT  (ext. pull-up)        │
                     │ GPIO 25 ◄── SW  (internal pull-up)     │
                     │                                        │
                     │ GPIO 2  ──► Onboard LED (WiFi status)  │
                     └────────────────────────────────────────┘
```

---

## Firmware State Machine

The ESP32 firmware runs a state machine. Understanding this helps diagnose what a node is doing at any moment.

```
                    Power on
                       │
                       ▼
                ┌─────────────┐
                │    BOOT     │ Init hardware, WiFi, MQTT, NVS
                └──────┬──────┘
                       │ Setup complete
                       ▼
              ┌────────────────┐
         ┌────►     IDLE       │◄────────────────────────────────┐
         │    │                │  OLED: "Tap card to start"      │
         │    └───────┬────────┘  LED: white breathing           │
         │            │ HC89: card detected                       │
         │            ▼                                           │
         │    ┌────────────────┐                                  │
         │    │ CARD_READING   │  OLED: spinner                  │
         │    └───────┬────────┘  LED: cyan flash                 │
         │            │                                           │
         │    ┌───────┴────────────────────────────┐             │
         │    │ Read result                        │             │
         │    ▼                                    ▼             │
         │  GRANTED                           DENIED             │
         │    │                          ┌────────────────┐      │
         │    │                          │ ACCESS_DENIED  │──────┘
         │    │                          │ (3s timeout)   │
         │    ▼                          └────────────────┘
         │  ┌─────────────────┐
         │  │ SESSION_ACTIVE  │  Relay ON, OLED: timer
         │  └────────┬────────┘  LED: solid green
         │           │
         │    ┌──────┴──────────────────┐
         │    │                         │
         │ card removed          2 min before limit
         │    │                         │
         │    │              ┌──────────────────────┐
         │    │              │  TIMEOUT_WARNING      │
         │    │              │  LED: yellow↔orange   │
         │    │              └──────────┬────────────┘
         │    │                    limit hit
         │    │                         │
         │    ▼                         ▼
         │  ┌─────────────────────────────────────┐
         │  │         SESSION_ENDED               │  Relay OFF
         │  │  Log event → MQTT → LittleFS        │  LED: off
         │  └───────────────┬─────────────────────┘
         │                  │ 3s
         └──────────────────┘

Any state + MQTT "stop"  → SESSION_ENDED
Any state + MQTT "ota"   → OTA_UPDATE (relay OFF first)
Any state + watchdog     → Hardware reset → BOOT
```

---

## MQTT Communication Architecture

### Topic map

| Topic | Direction | QoS | Retained | Payload |
|---|---|---|---|---|
| `mms/nodes/{id}/status` | Node → RPi | 1 | YES | `{state, roll, session_start, fw_version, ip, rssi}` |
| `mms/nodes/{id}/event` | Node → RPi | 1 | No | `{t, r, m, e, d, er}` |
| `mms/nodes/{id}/heartbeat` | Node → RPi | 0 | No | `{ts, heap_free, uptime_s}` |
| `mms/nodes/{id}/command` | RPi → Node | 1 | No | `{cmd, issued_by, ts}` |
| `mms/nodes/{id}/config` | RPi → Node | 1 | YES | `{name, session_limit_min, relay_active_high, machine_active}` |
| `mms/all/revoked` | RPi → All | 1 | YES | `{uids:["A1B2C3",...], updated_at:ts}` |
| `mms/bridge/status` | RPi → All | 1 | YES | `{state:"online"\|"offline"}` |

**Why retained messages matter:** When an ESP32 reboots and reconnects to MQTT, the broker immediately delivers the last retained `config` and `revoked` messages. The node gets its current configuration and revocation list without waiting for the admin to change anything. This is how offline-then-reconnect works seamlessly.

**Last Will Testament (LWT):** Each node sets a LWT before connecting. If it disconnects unexpectedly, the broker publishes `{"state":"offline"}` to `mms/nodes/{id}/status` (retained). The bridge sees this and the dashboard shows the node as offline.

### MQTT client IDs

Each client must have a unique ID. Duplicate IDs cause disconnection loops.

| Client | ID |
|---|---|
| Bridge | `bridge` |
| Node 1 | `mms_node_1` |
| Node 2 | `mms_node_2` |
| ... | ... |
| Node 15 | `mms_node_15` |

---

## Firebase Architecture

### Why two databases?

Firebase provides two real-time database products:
- **Firestore**: Document database optimised for complex queries, offline sync, and history. Used for permanent records (sessions, users, machines).
- **RTDB (Realtime Database)**: Simple JSON tree optimised for ultra-low latency live data. Used for real-time machine states, commands, and OTA triggers.

### Data flow: session event

```
ESP32 card removed
  → Session::end() called
  → Logger::appendSessionEnd() → /logs/sessions.jsonl (LittleFS)
  → MqttClient::publishEvent() → mms/nodes/3/event (MQTT QoS 1)
  → Bridge on_message() receives it
  → bridge.handle_node_event()
      → deduplication check (prevents duplicate on power-failure replay)
      → _fs_client.collection("sessions").add(session_data)   ← Firestore
      → _fs_client.collection("users").document(roll).update(last_seen)
  → Web app onSnapshot() triggers → dashboard updates
```

### Data flow: remote stop command

```
Admin clicks "Stop" in web app
  → setDoc(doc(rtdb, "/mms/commands/3"), {command:"stop", ts:now})
  → Bridge setup_rtdb_command_listener() fires
  → bridge checks: not stale (< 5 min), not acknowledged
  → mqtt_publish("mms/nodes/3/command", {cmd:"stop", ts:now}) QoS 1
  → Node MQTT callback receives it
  → processPendingCommand() in state_machine.cpp
  → Relay::off() → session ended → event published → RTDB cleared
```

### Data flow: OTA update

```
Super admin writes {firmware_url, target_version} to RTDB /mms/ota
  → Bridge setup_rtdb_ota_listener() fires
  → For each machine in Firestore /machines:
      → mqtt_publish("mms/nodes/{id}/command", {cmd:"ota", url, version})
  → Each node receives "ota" command
  → OTA::applyUpdate(url, version) called
  → esp_https_ota() downloads from Firebase Storage URL
  → On success: update NVS fw_version, ESP.restart()
  → If firmware crashes 3x on boot: rollback to previous partition
```

### Data flow: card revocation

```
Admin marks card as lost/stolen in web app
  → createDoc(doc(db, "revoked", uid), {reason:"lost"})
  → Bridge setup_firestore_revocation_listener() fires
  → Rebuild uid list from all /revoked docs
  → mqtt_publish("mms/all/revoked", {uids:[...], updated_at:ts}) retained
  → All connected nodes receive update → LittleFS /config/revoked.json updated
  → Next card tap with that UID: check → denied
```

---

## Access Control Architecture

The access decision is made entirely on the ESP32. No network call is needed.

```
Card inserted into slot
  ↓
HC89 slot sensor detects (GPIO 32 goes LOW)
  ↓
MFRC522 reads UID (SPI)
  ↓
Derive sector key:
  sector_key = HMAC-SHA256(master_key, uid_hex_uppercase)[0:6]
  (master_key is 32 bytes stored in NVS, never changes per deployment)
  ↓
Authenticate Sector 1, Block 7 with sector_key (MFRC522 MIFARE_Auth)
  ↓ (fails if card is from another system or was issued with wrong master key)
Read Block 4: roll_number[0:8] + schema_version[9]
Read Block 5: pin_hash[0:7] + perm_mask[8:11]
  ↓
Check 1: schema_version == 0x02 ?
  No → ACCESS DENIED ("Old card format")
  ↓
Check 2: uid in revocation list? (scan LittleFS /config/revoked.json)
  Yes → ACCESS DENIED ("Card revoked")
  ↓
Check 3: (perm_mask >> (MACHINE_ID - 1)) & 1 == 1 ?
  No → ACCESS DENIED ("No permission")
  ↓
Check 4: NVS::getMachineActive() == true ?
  No → ACCESS DENIED ("Machine disabled")
  ↓
ACCESS GRANTED → Relay::on()
```

**Key point:** This entire process takes ~200ms and requires zero network access. The node can grant access even if WiFi is completely down.

---

## Offline Operation Architecture

The system degrades gracefully during network failures:

```
WiFi or MQTT down
  ├── Access decisions: UNCHANGED (still fully functional)
  ├── Session logging: written to LittleFS /logs/sessions.jsonl
  ├── Revocation updates: cannot receive new revocations
  │     (last known list still enforced)
  ├── Config updates: cannot receive new session limits
  │     (NVS cached values used)
  └── Status display: 128×32 OLED shows "OFFLINE" indicator

WiFi or MQTT restored
  ├── LittleFS logger flushes buffered events to MQTT (QoS 1)
  ├── Broker delivers retained config + revoked messages
  ├── Node resumes publishing status + heartbeats
  └── Bridge processes buffered events into Firestore
```

**Offline capacity:** At 120 sessions/day average, LittleFS (1.5MB partition) holds approximately **5,000 session records** before rotation. Practically unlimited offline resilience for a lab.

---

## NVS (Non-Volatile Storage) Architecture

The ESP32 stores its configuration in NVS flash (survives reboots and power loss):

| NVS Key | Type | Set By | Description |
|---|---|---|---|
| `machine_id` | uint8 | Provisioning (config.h) | 1–32; fixed per node |
| `master_key` | blob 32B | Provisioning (secrets.h) | Shared secret for key derivation |
| `machine_name` | string | MQTT config push | Human-readable name |
| `session_limit` | uint16 | MQTT config push | Minutes; fallback to config.h |
| `relay_active_high` | uint8 | MQTT config push | Relay polarity |
| `machine_active` | uint8 | MQTT config push | 0 = admin-disabled |
| `fw_version` | string | OTA handler | Current firmware version |

Config pushed via MQTT is stored in NVS immediately, so it persists across reboots even if the node comes back up while the RPi is offline.
