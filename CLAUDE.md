# CLAUDE.md — MMS v2 AI Reference

Machine Management System v2 for Tinkerers' Lab, IIT Indore.
RFID-based machine access control: ESP32 nodes + RPi bridge + Firebase + React dashboard.

---

## Repository Layout

```
firmware/
  access_node/           ESP32 production firmware (Arduino/C++)
  hardware_validation/   12 test sketches, one per hardware module

apps/
  kiosk_station/         Flask card-issuing kiosk + Arduino RFID writer firmware
  web_app/               React/Vite/Tailwind/shadcn-ui admin dashboard (PWA)

services/
  rpi_bridge/            Python MQTT↔Firebase bridge (runs on Raspberry Pi)

firebase/
  firestore.rules        Firestore security rules
  firestore.indexes.json Composite index definitions
  database.rules.json    Realtime Database rules

docs/
  deployment/            17-doc deployment runbook (numbered sequence)
  handbook/              LaTeX source for formal project handbook (v2.1)

scripts/
  seed_machines.py       Populates Firestore machines collection

archive/
  v1/                    MMS v1 source, CAD, docs, media — read-only reference
  handbook_v2.0/         Earlier handbook LaTeX source
```

---

## Key Secrets (gitignored, never commit)

| File | Contents |
|---|---|
| `firmware/access_node/secrets.h` | WiFi SSID/pass, MQTT host/user/pass, 32-byte master key |
| `apps/kiosk_station/secrets.py` | 32-byte master key |
| `apps/kiosk_station/station_writer/secrets_station.h` | 32-byte master key |
| `services/rpi_bridge/.env` | Firebase project ID, MQTT credentials, service account path |
| `services/rpi_bridge/service-account.json` | Firebase Admin SDK key |

The 32-byte master key must be identical in all three locations. Cards issued by the kiosk use CRYPTO1 encoded with this key; access nodes validate using the same key.

---

## Firmware: `firmware/access_node/`

- Language: Arduino/C++ for ESP32
- Entry: `access_node.ino`
- Per-node config: `config.h` (MACHINE_ID, MACHINE_NAME, pin assignments)
- Secrets: `secrets.h` (copy from `secrets.h.template`)
- State machine in `state_machine.cpp` drives the main loop
- RFID read + CRYPTO1 auth: `rfid_handler.cpp` (uses MFRC522 library)
- Relay control: `relay_controller.cpp`
- MQTT pub/sub: `mqtt_client.cpp` (PubSubClient library)
- Session lifecycle: `session_manager.cpp`
- Offline revocation: `revocation_cache.cpp` (NVS-backed)
- Local log: `littlefs_logger.cpp` (LittleFS)
- OTA: `ota_handler.cpp`
- Displays: `display_primary.cpp` (128×64 SPI OLED), `display_status.cpp` (128×32 I2C)

### Arduino IDE settings for ESP32

- Board: ESP32 Dev Module
- Upload Speed: 921600
- CPU Frequency: 240 MHz
- Partition Scheme: Default 4MB with spiffs
- ESP32 core: 3.1.x (Espressif)

### Libraries (install via Library Manager)

Adafruit SSD1306 2.5.9, Adafruit GFX 1.11.9, MFRC522 1.4.11, Adafruit NeoPixel 1.12.3, PubSubClient 2.8.0, ArduinoJson 7.3.1

---

## Bridge Service: `services/rpi_bridge/`

- Language: Python 3.11+
- Entry: `bridge.py`
- Deps: `requirements.txt` (paho-mqtt, firebase-admin)
- Config: `.env` (copy from `.env.template`)
- MQTT broker: Mosquitto, config in `mosquitto.conf`
- Systemd unit: `mms-bridge.service`
- Credential helper: `generate_mqtt_creds.py`

---

## Kiosk: `apps/kiosk_station/`

- Language: Python 3.11+, Flask
- Entry: `app.py`
- Config: `config.py` (machine list, Flask settings)
- Firebase client: `firebase_config.py`
- RFID encoding: `rfid_handler.py`
- Secrets: `secrets.py` (copy from `secrets.py.template`)
- UI template: `templates/index.html`
- RFID writer firmware: `station_writer/station_writer.ino` (Arduino for the kiosk's attached reader/writer)

---

## Web Dashboard: `apps/web_app/`

- React 18, Vite, TypeScript, Tailwind CSS, shadcn/ui
- Firebase Auth + Firestore + RTDB (real-time machine status)
- Entry: `src/main.tsx`
- Pages: `src/pages/` (Index, Login, admin, super-admin, NotFound)
- Key components: Dashboard, MachineManagement, UserManagement, UsageLogs, AddUserForm, SuperAdminPanel, PublicDashboard
- Firebase context: `src/contexts/Firebase.tsx`
- Machine context: `src/contexts/Machines.tsx`
- Dev: `npm run dev` inside `apps/web_app/`
- Build: `npm run build` → `dist/`

---

## Firebase

- Firestore: sessions, users, machines, access grants
- RTDB: live machine status (online/offline, active session)
- Auth: email/password for admin dashboard users
- Storage: OTA firmware binaries
- Rules + indexes: `firebase/` folder
- Deploy: `firebase deploy --only firestore:rules,firestore:indexes,database`

---

## Hardware Validation: `firmware/hardware_validation/`

Run sketches 01–12 in order on every node before provisioning. Sign off in `EXECUTION_PLAN.md`. Integration test: `99_full_node/99_full_node.ino`.

---

## Deployment Docs: `docs/deployment/`

17 numbered markdown files. Always edit in sync with code changes:

| # | Topic |
|---|---|
| 01 | Executive overview |
| 02 | System architecture |
| 03 | Bill of materials |
| 04 | Machine registry (how to add/remove machines) |
| 05–06 | Hardware assembly and validation |
| 07 | Raspberry Pi setup |
| 08 | Firebase setup |
| 09 | Web app deployment |
| 10 | Card kiosk setup |
| 11 | Access node provisioning (flash + configure) |
| 12 | Integration testing |
| 13 | Production rollout |
| 14 | Maintenance |
| 15 | Troubleshooting |
| 16 | Disaster recovery |
| 17 | Future expansion |

When adding a new machine: update `firmware/access_node/config.h`, `apps/kiosk_station/config.py`, and Firestore via `scripts/seed_machines.py`. See doc 04 and 17.

---

## gitignore Rules

- `firmware/access_node/secrets.h` — never commit
- `apps/kiosk_station/secrets.py` — never commit
- `apps/kiosk_station/station_writer/secrets_station.h` — never commit
- `services/rpi_bridge/.env` and `service-account.json` — never commit
- `apps/web_app/.env` — never commit
- LaTeX build artifacts in `docs/handbook/build/` and `archive/handbook_v2.0/build/` — gitignored
- `node_modules/`, `dist/`, `__pycache__/`, `.pyc` — gitignored

---

## Common Tasks

**Add a new machine:**
1. Edit `firmware/access_node/config.h` with new MACHINE_ID/MACHINE_NAME
2. Edit `apps/kiosk_station/config.py` MACHINE_IDS dict
3. Run `scripts/seed_machines.py` to add Firestore entry
4. Flash new ESP32 node from `firmware/access_node/`

**Issue a new card:**
Run the kiosk app (`apps/kiosk_station/`), select user, tap card on writer.

**Update bridge service:**
Edit `services/rpi_bridge/bridge.py`, copy to RPi, `sudo systemctl restart mms-bridge`.

**Rebuild handbook:**
```sh
cd docs/handbook
pdflatex -output-directory=build main.tex
```
The compiled PDF in `build/` is gitignored; commit only the `.tex` source and the root-level `TLMMS_Handbook.pdf`.

**Deploy updated Firebase rules:**
```sh
firebase deploy --only firestore:rules,firestore:indexes,database
```
