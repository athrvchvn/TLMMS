# Machine Management System v2

RFID-based access control and machine usage management system for **Tinkerers' Lab, IIT Indore**.

MMS v2 lets lab administrators issue RFID cards to members, restrict machine access by permission level, log every session, and monitor all machines from a web dashboard — all with offline-capable hardware at each machine.

---

## System Overview

Four runtime components work together:

| Component | What it does |
|---|---|
| **ESP32 access node** | Mounted on each machine. Reads RFID cards, controls the relay, logs sessions. |
| **Raspberry Pi bridge** | Runs Mosquitto + `bridge.py`. Connects node MQTT traffic to Firebase. |
| **Card issuing kiosk** | Flask web app + Arduino RFID writer. Issues and encodes member cards. |
| **Web admin dashboard** | React/Vite PWA. Manages users, machines, permissions, and audit logs. |

Firebase stores system state (Firestore + RTDB). MQTT handles local node-to-bridge communication.

---

## Repository Structure

```
├── firmware/
│   ├── access_node/         ESP32 production firmware (Arduino/C++)
│   └── hardware_validation/ Standalone test sketches for each hardware module
│
├── apps/
│   ├── kiosk_station/       Flask card-issuing kiosk + RFID writer firmware
│   └── web_app/             React/Vite/Tailwind admin dashboard
│
├── services/
│   └── rpi_bridge/          Raspberry Pi MQTT↔Firebase bridge (Python)
│
├── firebase/
│   ├── firestore.rules      Firestore security rules
│   ├── firestore.indexes.json  Composite index definitions
│   └── database.rules.json  Realtime Database security rules
│
├── docs/
│   ├── deployment/          Step-by-step deployment and maintenance handbook (17 docs)
│   └── handbook/            LaTeX source for the formal project handbook (v2.1)
│
├── scripts/
│   └── seed_machines.py     Seeds the machine registry into Firestore
│
└── archive/
    ├── v1/                  MMS v1 source, CAD files, docs, and media
    └── handbook_v2.0/       Earlier handbook version (LaTeX source)
```

---

## Quick Start

### Prerequisites

- Firebase project (Firestore + RTDB + Storage + Auth enabled)
- Raspberry Pi 4 or 3B+ with Ethernet
- ESP32 Dev Module per machine
- MFRC522 RFID reader per machine
- Node.js 18+ and Python 3.11+ on your laptop

### Deployment Order

Follow `docs/deployment/` in numbered order. Start here:

```
01 → Read executive overview
03 → Confirm bill of materials
08 → Set up Firebase
07 → Set up Raspberry Pi bridge
09 → Deploy web dashboard
10 → Set up card issuing kiosk
06 → Validate hardware on first node
11 → Flash and provision first ESP32 node
12 → Run integration tests
13 → Roll out to remaining machines
```

Full guide: [docs/deployment/README.md](docs/deployment/README.md)

---

## Component Guide

### `firmware/access_node/`

Production ESP32 firmware. Each machine gets one flashed node.

Key files:

| File | Purpose |
|---|---|
| `access_node.ino` | Main sketch entry point |
| `config.h` | Machine ID, name, pin assignments — edit per node |
| `secrets.h.template` | Copy to `secrets.h` and fill in WiFi/MQTT/master key |
| `state_machine.*` | Node state transitions |
| `rfid_handler.*` | Card read and CRYPTO1 validation |
| `relay_controller.*` | Machine power relay control |
| `session_manager.*` | Session lifecycle (start, extend, end) |
| `mqtt_client.*` | MQTT pub/sub layer |
| `revocation_cache.*` | Local cache of revoked cards (offline-safe) |
| `littlefs_logger.*` | On-device log storage |
| `ota_handler.*` | Over-the-air firmware update support |

**Setup:** copy `secrets.h.template` → `secrets.h`, fill in values. The real `secrets.h` is gitignored and must never be committed.

### `firmware/hardware_validation/`

12 standalone Arduino sketches — one per hardware module. Run these in order on every freshly assembled node before flashing production firmware.

```
01_esp32_system    02_mfrc522        03_hc89_slot_sensor
04_oled64_spi      05_oled32_i2c     06_relay_mechanical
07_relay_ssr       08_ws2812_led     09_rotary_encoder
10_wifi            11_mqtt           12_littlefs
```

See [firmware/hardware_validation/EXECUTION_PLAN.md](firmware/hardware_validation/EXECUTION_PLAN.md) for the sign-off checklist.

### `services/rpi_bridge/`

Python service on the Raspberry Pi. Bridges local MQTT from ESP32 nodes to Firebase (Firestore sessions, RTDB machine status, command delivery).

```sh
cd services/rpi_bridge
cp .env.template .env          # fill in Firebase project ID, MQTT creds
pip install -r requirements.txt
python bridge.py
# or install as a systemd service via mms-bridge.service
```

### `apps/kiosk_station/`

Flask web app for issuing RFID cards at the lab front desk. Talks to Firebase to look up members and write CRYPTO1-encoded card payloads via a connected RFID writer.

```sh
cd apps/kiosk_station
cp secrets.py.template secrets.py   # fill in master key
pip install -r requirements.txt
python app.py
```

The `station_writer/` subfolder contains Arduino firmware for the RFID writer attached to the kiosk PC.

**The master key in `kiosk_station/secrets.py` must exactly match the one in every `access_node/secrets.h`. A mismatch means issued cards will not validate on nodes.**

### `apps/web_app/`

React/Vite admin dashboard. Supports user management, machine management, permission grants, session audit logs, and real-time status monitoring. Built as a PWA.

```sh
cd apps/web_app
npm install
npm run dev       # development server
npm run build     # production build → dist/
```

### `firebase/`

Firebase security rules and index definitions. Deploy from the repo root:

```sh
firebase deploy --only firestore:rules
firebase deploy --only firestore:indexes
firebase deploy --only database
```

### `docs/deployment/`

Complete operational runbook — 17 documents covering every deployment step from hardware assembly to disaster recovery. Written for a junior engineer with no prior MMS experience.

### `scripts/`

```sh
python scripts/seed_machines.py    # populate Firestore machines collection
```

---

## Secrets Reference

Never commit real secrets. Use the provided templates:

| Template | Copy to | Who reads it |
|---|---|---|
| `firmware/access_node/secrets.h.template` | `firmware/access_node/secrets.h` | ESP32 nodes |
| `apps/kiosk_station/secrets.py.template` | `apps/kiosk_station/secrets.py` | Kiosk app |
| `apps/kiosk_station/station_writer/secrets_station.h.template` | `…/secrets_station.h` | RFID writer firmware |
| `services/rpi_bridge/.env.template` | `services/rpi_bridge/.env` | Bridge service |

All three locations that use the master key must have **identical** values:
1. `firmware/access_node/secrets.h` — `MASTER_KEY`
2. `apps/kiosk_station/secrets.py` — `MASTER_KEY`
3. `apps/kiosk_station/station_writer/secrets_station.h` — `MASTER_KEY`

Pre-push checklist:

```sh
git status
git ls-files | grep -E 'secrets|service-account|\.env$'
```

---

## Archive

`archive/v1/` — MMS v1 access node code, kiosk app, CAD enclosure files, and media. Historical reference only; do not develop here.

`archive/handbook_v2.0/` — Earlier LaTeX handbook source.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Node firmware | Arduino/C++ on ESP32 |
| RFID protocol | MFRC522 with MIFARE CRYPTO1 |
| Local messaging | MQTT (Mosquitto on Raspberry Pi) |
| Bridge service | Python 3, paho-mqtt, firebase-admin |
| Cloud backend | Firebase (Firestore, RTDB, Auth, Storage) |
| Web dashboard | React 18, Vite, TypeScript, Tailwind CSS, shadcn/ui |
| Card kiosk | Flask, Python |
