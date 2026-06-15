# Machine Management System v2

Machine Management System v2, or MMS v2, is an RFID-based access control and machine usage management system for Tinkerers' Lab, IIT Indore. The repository contains the complete working stack: ESP32 access node firmware, a Raspberry Pi bridge service, a card issuing kiosk, a React web dashboard, Firebase rules, hardware validation sketches, deployment documentation, and archived v1 material.

This README documents the current folder structure as it exists in this repository.

## System Overview

MMS v2 is made of four main runtime components:

1. **ESP32 access nodes** installed on individual machines.
2. **Raspberry Pi bridge** that connects MQTT node traffic with Firebase.
3. **Card issuing kiosk** used to create RFID access cards.
4. **Web admin dashboard** used to manage users, machines, access, and monitoring.

Firebase stores the system state, access policies, sessions, audit logs, and machine registry. MQTT is used for local communication between ESP32 nodes and the Raspberry Pi bridge.

## Repository Structure

```text
v2/
├── access_node/             ESP32 firmware for production machine access nodes
├── deployment/              Step-by-step deployment and maintenance handbook
├── handbook2.0/             Older LaTeX handbook source and generated files
├── handbook2.1/             Current LaTeX handbook source and generated files
├── hardware_validation/     Arduino sketches for validating hardware modules
├── kiosk_station/           Flask app and RFID writer firmware for card issuing
├── rpi_bridge/              Raspberry Pi MQTT/Firebase bridge service
├── scripts/                 Utility scripts for setup and data seeding
├── v1/                      Archived MMS v1 source, docs, media, and CAD files
├── web_app/                 React/Vite admin dashboard
├── database.rules.json      Firebase Realtime Database security rules
├── firestore.indexes.json   Firestore composite index definitions
├── firestore.rules          Firestore security rules
└── .gitignore               Secrets, Python cache, and editor ignore rules
```

## Folder Guide

### `access_node/`

Production ESP32 firmware for the machine-side access controller.

Important files:

```text
access_node/
├── access_node.ino          Main Arduino sketch entry point
├── config.h                 Machine ID, machine name, pins, feature settings
├── secrets.h.template       Template for WiFi, MQTT, and master key secrets
├── mqtt_client.*            MQTT communication layer
├── rfid_handler.*           RFID card read/validation logic
├── relay_controller.*       Machine power relay control
├── session_manager.*        Access session lifecycle handling
├── state_machine.*          Node state transitions
├── display_primary.*        Primary OLED display output
├── display_status.*         Secondary/status display output
├── revocation_cache.*       Local card/user revocation cache
├── littlefs_logger.*        Local filesystem logging
├── nvs_manager.*            ESP32 non-volatile storage helpers
└── ota_handler.*            Over-the-air update support
```

Before flashing a node, copy `access_node/secrets.h.template` to `access_node/secrets.h` and fill in the required local values. The real `secrets.h` file must not be committed.

### `hardware_validation/`

Standalone Arduino sketches used to validate each hardware module before final integration.

```text
hardware_validation/
├── 01_esp32_system/
├── 02_mfrc522/
├── 03_hc89_slot_sensor/
├── 04_oled64_spi/
├── 05_oled32_i2c/
├── 06_relay_mechanical/
├── 07_relay_ssr/
├── 08_ws2812_led/
├── 09_rotary_encoder/
├── 10_wifi/
├── 11_mqtt/
├── 12_littlefs/
├── EXECUTION_PLAN.md
└── VALIDATION_GUIDE.md
```

Use this folder before provisioning production nodes. The validation documents explain the intended test order and sign-off process.

### `rpi_bridge/`

Python service that runs on the Raspberry Pi. It connects local MQTT traffic from ESP32 nodes with Firebase services.

Important files:

```text
rpi_bridge/
├── bridge.py                Main bridge application
├── requirements.txt         Python dependencies
├── .env.template            Environment variable template
├── service-account.json     Firebase Admin SDK key, local only
├── mms-bridge.service       systemd service file
├── mosquitto.conf           MQTT broker configuration
├── setup.sh                 Raspberry Pi setup helper
└── generate_mqtt_creds.py   MQTT credential generation helper
```

Local runtime files such as `rpi_bridge/.env` and `rpi_bridge/service-account.json` should never be committed.

### `kiosk_station/`

Card issuing station. This includes a Flask web app plus Arduino firmware for the RFID writer attached to the kiosk.

Important files:

```text
kiosk_station/
├── app.py                         Flask kiosk application
├── config.py                      Kiosk machine registry and app settings
├── firebase_config.py             Firebase client setup
├── models.py                      Data models
├── rfid_handler.py                RFID/card encoding logic
├── requirements.txt               Python dependencies
├── secrets.py.template            Template for kiosk secrets
├── templates/index.html           Kiosk UI template
└── station_writer/
    ├── station_writer.ino         Arduino firmware for RFID writer
    └── secrets_station.h.template Writer firmware secret template
```

The master key used here must match the master key used by the ESP32 access nodes.

### `web_app/`

React/Vite admin dashboard for managing MMS from a browser.

Important files and folders:

```text
web_app/
├── src/                    React application source
├── public/                 Static assets, icons, manifest, uploads
├── supabase/               Supabase local config folder
├── package.json            npm scripts and dependencies
├── package-lock.json       npm dependency lockfile
├── vite.config.ts          Vite configuration
├── tailwind.config.ts      Tailwind CSS configuration
├── components.json         shadcn/ui configuration
└── README.md               Original Lovable project README
```

Common commands:

```sh
cd web_app
npm install
npm run dev
npm run build
```

### `deployment/`

The main operational documentation set. Start here when deploying MMS v2.

```text
deployment/
├── README.md
├── 01_executive_overview.md
├── 02_system_architecture.md
├── 03_bill_of_materials.md
├── 04_machine_registry.md
├── 05_hardware_assembly.md
├── 06_hardware_validation.md
├── 07_raspberry_pi_deployment.md
├── 08_firebase_deployment.md
├── 09_web_application_deployment.md
├── 10_card_issuing_station.md
├── 11_access_node_provisioning.md
├── 12_integration_testing.md
├── 13_production_rollout.md
├── 14_maintenance_handbook.md
├── 15_troubleshooting_guide.md
├── 16_disaster_recovery.md
└── 17_future_expansion.md
```

Recommended first-time reading order:

1. `deployment/01_executive_overview.md`
2. `deployment/02_system_architecture.md`
3. `deployment/03_bill_of_materials.md`
4. `deployment/04_machine_registry.md`
5. `deployment/08_firebase_deployment.md`
6. `deployment/07_raspberry_pi_deployment.md`
7. `deployment/09_web_application_deployment.md`
8. `deployment/10_card_issuing_station.md`
9. `deployment/11_access_node_provisioning.md`
10. `deployment/12_integration_testing.md`

### `handbook2.1/`

Current LaTeX handbook source and compiled PDF artifacts.

```text
handbook2.1/
├── main.tex
├── main.pdf
├── appendices/
├── chapters/
├── figures/
├── frontmatter/
└── build/
```

Use this folder when editing or rebuilding the formal project handbook.

### `handbook2.0/`

Older handbook version. Keep this for historical reference unless the team decides to archive or remove old generated files.

### `scripts/`

Utility scripts for setup and maintenance.

```text
scripts/
└── seed_machines.py        Seeds the machine registry into Firebase/Firestore
```

### `v1/`

Archived MMS v1 material. This includes old access node files, kiosk station code, web app code, docs, CAD files, and media.

This folder is useful for historical reference, but new development should happen in the v2 folders listed above.

## Firebase Files

The Firebase rules and index files currently live at the repository root:

```text
firestore.rules             Firestore access rules
firestore.indexes.json      Firestore composite indexes
database.rules.json         Realtime Database rules
```

These are used during Firebase deployment and should be kept in sync with the web app, Raspberry Pi bridge, and deployment documentation.

## Secrets And Local-Only Files

Do not commit real secrets. Use the provided templates instead.

```text
access_node/secrets.h.template
kiosk_station/secrets.py.template
kiosk_station/station_writer/secrets_station.h.template
rpi_bridge/.env.template
```

Local files that should remain untracked:

```text
access_node/secrets.h
kiosk_station/secrets.py
kiosk_station/station_writer/secrets_station.h
rpi_bridge/.env
rpi_bridge/service-account.json
```

The same 32-byte master key must be used by:

1. `access_node/secrets.h`
2. `kiosk_station/secrets.py`
3. `kiosk_station/station_writer/secrets_station.h`

If these values do not match, cards issued by the kiosk will not validate correctly on the access nodes.

## Suggested Development Workflow

For a new deployment or major test cycle:

1. Review `deployment/README.md`.
2. Configure Firebase using `firestore.rules`, `firestore.indexes.json`, and `database.rules.json`.
3. Seed or verify the machine registry using `scripts/seed_machines.py`.
4. Set up the Raspberry Pi bridge from `rpi_bridge/`.
5. Build and deploy the admin dashboard from `web_app/`.
6. Set up the card issuing station from `kiosk_station/`.
7. Validate each hardware module using `hardware_validation/`.
8. Configure and flash production node firmware from `access_node/`.
9. Run integration tests from `deployment/12_integration_testing.md`.

## Uploading To GitHub

Before pushing this repository to GitHub:

1. Confirm no real secrets are present.
2. Check `git status` for deleted or unexpected files.
3. Decide whether generated handbook build files should remain tracked.
4. Keep `v1/` only if historical source and CAD files are useful to future maintainers.
5. Make sure `web_app/node_modules/`, Python caches, local `.env` files, and Firebase service account keys are not committed.

Useful checks:

```sh
git status
git ls-files | grep -E 'secrets|service-account|\.env$'
```

## Current Structure Notes

The current repository is intentionally broad: it contains firmware, backend bridge code, a kiosk app, a web app, deployment documentation, formal handbook sources, and legacy v1 material in one place. This is useful for deployment because all system components are versioned together.

If the repository is later cleaned up, a natural next step would be to group source folders under categories such as `firmware/`, `apps/`, `services/`, `docs/`, `firebase/`, and `archive/`. For now, this README documents the existing layout without requiring any folder moves.
