# Section 01 — Executive Overview

→ Next: [System Architecture](02_system_architecture.md)

---

## What Is MMS V2?

MMS (Machine Management System) Version 2 is an RFID-based access control and session logging system for the Tinkerers' Lab at IIT Indore. It controls which students can use which lab machines, records how long each machine was used and by whom, and allows lab administrators to manage access remotely through a web dashboard.

**In practical terms:** A student walks up to a machine (e.g., the Creality 3D Printer). They insert their RFID card into the slot. The machine reads the card, verifies the student is authorised to use that specific machine, and if authorised, turns on the power relay to the machine. The student uses the machine. When done, they remove the card and the relay switches off. The session is recorded in Firebase and is visible on the admin dashboard.

---

## Why MMS V2 Exists — V1 Failures

V1 was an earlier system that **couldn't worked end-to-end**. The system had systemic issues that prevented deployment:

| V1 Problem | Impact |
|---|---|
|
| PINs stored as plaintext in Firebase | Security vulnerability |
| Non-unique MQTT client IDs | Multiple nodes disconnected each other |
| No watchdog timer | Firmware lockup required manual reset |
| No OTA update mechanism | Every firmware change required physical access |
| Blocking `delay()` calls in main loop | System unresponsive during delays |
| No offline fallback | WiFi outage = system offline |
| No security rules on Firebase | Any device on the internet could read/write all data |

V2 is a ground-up redesign that addresses every V1 failure.

---

## V2 Key Improvements

| Feature | V1 | V2 |
|---|---|---|
| RFID card schema | Block 1+2 (default sector key) | Sector 1, blocks 4+5, HMAC-SHA256 derived key per card |
| Access decision | Network call required | Fully local on ESP32 — works offline |
| Session logging | Never reached Firestore | LittleFS buffer → MQTT → Firebase bridge |
| Offline operation | System offline | Continues granting/denying access; logs buffered |
| Firmware updates | Physical access required | OTA via Firebase Storage URL |
| Watchdog | None | 30-second hardware watchdog timer |
| Security | No Firestore rules; plaintext PINs | Full security rules; HMAC-SHA256 PIN hash |
| Remote control | None | Admin can stop a session or disable a machine from the web |
| Machine configuration | Hardcoded in firmware | Pushed via MQTT retained messages from Firebase |

---

## System Capabilities

### What the system can do
- Grant or deny machine access based on RFID card permissions
- Log session start, end, duration, and reason to Firebase
- Support up to 32 machines (uint32 permission bitmask per card)
- Operate fully offline if WiFi/MQTT is unavailable
- Buffer offline session logs and upload when connectivity restores
- Push firmware updates to all nodes simultaneously (OTA)
- Allow an admin to remotely stop an active session
- Allow an admin to remotely disable a machine (denies all access)
- Revoke a lost or stolen card within seconds of admin action
- Push configuration changes (session limits, machine names) live to nodes
- Show real-time machine status on the admin web dashboard

### What the system cannot do (V2 scope)
- PIN verification at the machine (deferred to Phase 4 — rotary encoder not yet installed)
- Machine reservation/booking
- Multi-lab deployment (each lab needs its own Firebase project)
- 5 GHz WiFi (ESP32 is 2.4 GHz only)

---

## Architecture Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                    RFID CARD (MIFARE Classic 1K)                │
│  Carries: Roll number, permissions bitmask, PIN hash            │
│  Secured by: Per-card HMAC-SHA256 sector key                    │
└───────────────────────────┬─────────────────────────────────────┘
                            │ Physical insertion into slot
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                  ESP32 ACCESS NODE (per machine)                │
│  Reads card → checks permissions → controls relay               │
│  Logs session to LittleFS → publishes to MQTT                   │
│  Updates two OLEDs + WS2812 LED                                 │
└───────────────────────────┬─────────────────────────────────────┘
                            │ MQTT over LAN (port 1883)
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│               RASPBERRY PI (static IP on lab LAN)              │
│  Runs: Mosquitto MQTT broker + bridge.py service                │
│  Bridge: routes MQTT ↔ Firebase (Firestore + RTDB)             │
└────────────────┬──────────────────────────────────┬────────────┘
                 │ Firebase Admin SDK               │ Admin SDK
                 ▼                                  ▼
┌─────────────────────────┐        ┌────────────────────────────┐
│  FIRESTORE              │        │  REALTIME DATABASE (RTDB)  │
│  /sessions              │        │  /mms/nodes/{id}  (status) │
│  /users                 │        │  /mms/commands/{id}        │
│  /machines              │        │  /mms/ota                  │
│  /revoked               │        │  /mms/revoked              │
└─────────────────────────┘        └────────────────────────────┘
                 │                                  │
                 └──────────────┬───────────────────┘
                                │ Firebase JS SDK
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│               REACT WEB APPLICATION (Firebase Hosting)          │
│  Admin dashboard: real-time status, session logs, user mgmt     │
│  Super admin: OTA trigger, machine enable/disable, revocation  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Deployment Scale Assumptions

This deployment package assumes:
- **10–15 machines** in a single lab
- **~300 users** (students with RFID cards)
- **1 Raspberry Pi** serving as both MQTT broker and Firebase bridge
- **1 WiFi access point** covering the lab (2.4 GHz, SSID: `tlmms`)
- **1 Firebase project** (Spark/Blaze plan — see Section 8 for cost)
- **1 card issuing station** (kiosk) at the lab entrance
- Machines operate in a single physical location — no multi-site deployment

If these assumptions change (e.g., 2 labs, >32 machines, >1 AP), see [Section 17 — Future Expansion](17_future_expansion.md).

---

## Terminology Glossary

| Term | Meaning |
|---|---|
| Access node | The ESP32 + peripherals mounted at each machine |
| Bridge | The Python service on the RPi that connects MQTT to Firebase |
| Card / RFID card | MIFARE Classic 1K card issued to each student |
| Kiosk | The card issuing station (RPi or laptop + MFRC522 + Flask web UI) |
| Machine ID | Integer 1–32 identifying a specific lab machine |
| Master key | 32-byte secret known to kiosk and all nodes; used for HMAC key derivation |
| MQTT | Lightweight message protocol used for LAN communication |
| Node | Short for "access node" |
| OTA | Over-the-air firmware update |
| Permission bitmask | 32-bit integer on the card; bit N-1 = permission for machine N |
| Sector key | 6-byte key derived per-card by HMAC-SHA256(master_key, card_UID) |
| Session | A single machine-use event: card inserted → card removed or timeout |
| RTDB | Firebase Realtime Database |
| Roll number | Student identifier (9-digit string, e.g., "220002123") |
