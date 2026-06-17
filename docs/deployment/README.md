# MMS V2 — Deployment Package Index

**Machine Management System Version 2**
Tinkerers' Lab, IIT Indore

---

## Who This Package Is For

This package is written for a junior engineer who has never seen MMS before. You need:
- Basic Arduino IDE experience (upload a sketch, open Serial Monitor)
- Basic Raspberry Pi Linux experience (SSH, install packages, edit files)
- Basic web development experience (edit `.env` files, run `npm` commands)

You do **not** need to understand the full codebase. Each handbook is self-contained.

---

## Document Map

| # | Document | When You Need It |
|---|---|---|
| [01](01_executive_overview.md) | Executive Overview | Before anything — read first |
| [02](02_system_architecture.md) | System Architecture | When you need to understand how pieces connect |
| [03](03_bill_of_materials.md) | Bill of Materials | Before ordering hardware |
| [04](04_machine_registry.md) | Machine Registry | Before flashing any node |
| [05](05_hardware_assembly.md) | Hardware Assembly | Before validating hardware |
| [06](06_hardware_validation.md) | Hardware Validation | After assembly, before integration |
| [07](07_raspberry_pi_deployment.md) | Raspberry Pi Deployment | Setting up the RPi bridge |
| [08](08_firebase_deployment.md) | Firebase Deployment | Setting up cloud backend |
| [09](09_web_application_deployment.md) | Web Application Deployment | Deploying the admin dashboard |
| [10](10_card_issuing_station.md) | Card Issuing Station | Setting up the kiosk |
| [11](11_access_node_provisioning.md) | Access Node Provisioning | Flashing and configuring each ESP32 node |
| [12](12_integration_testing.md) | Integration Testing | Verifying everything works together |
| [13](13_production_rollout.md) | Production Rollout Plan | Staged deployment to all machines |
| [14](14_maintenance_handbook.md) | Maintenance Handbook | Ongoing operation |
| [15](15_troubleshooting_guide.md) | Troubleshooting Guide | When something breaks |
| [16](16_disaster_recovery.md) | Disaster Recovery | When something fails catastrophically |
| [17](17_future_expansion.md) | Future Expansion | Adding machines, features |

---

## First-Time Deployment Order

Follow this order exactly. Do not skip steps.

```
Step 1  Read Executive Overview (01)
Step 2  Confirm BOM and procure hardware (03)
Step 3  Set machine registry (04)
Step 4  Set up Firebase (08) ← requires Google account
Step 5  Set up Raspberry Pi (07)
Step 6  Deploy web application (09)
Step 7  Set up card issuing station (10)
Step 8  Assemble first node hardware (05)
Step 9  Validate first node hardware (06)
Step 10 Provision first node (11)
Step 11 Run integration test on first node (12)
Step 12 Pilot deployment — 1 machine (13)
Step 13 Expand to 5 machines (13)
Step 14 Full rollout — 15 machines (13)
```

---

## Critical Files In The Repository

```
firmware/
├── access_node/            ← ESP32 firmware source
│   ├── config.h            ← Machine ID, name, pin assignments (edit per node)
│   ├── secrets.h           ← WiFi, MQTT, master key (NEVER commit, gitignored)
│   └── secrets.h.template  ← Copy this to secrets.h and fill in values
└── hardware_validation/    ← Standalone test sketches (run before provisioning)

services/
└── rpi_bridge/
    ├── bridge.py           ← Main bridge service
    ├── .env.template       ← Copy to .env and fill in values
    └── mms-bridge.service  ← systemd unit file

apps/
├── kiosk_station/
│   ├── app.py              ← Flask kiosk application
│   ├── secrets.py          ← Master key (NEVER commit, gitignored)
│   ├── secrets.py.template ← Copy this to secrets.py
│   └── station_writer/     ← Arduino firmware for RFID writer
└── web_app/                ← React admin dashboard

firebase/
├── firestore.rules         ← Firestore security rules
├── database.rules.json     ← RTDB security rules
└── firestore.indexes.json  ← Required composite indexes

scripts/
└── seed_machines.py        ← One-time Firestore population
```

---

## Quick Reference — Secrets Locations

| Secret | File | Who Uses It |
|---|---|---|
| WiFi SSID/Password | `firmware/access_node/secrets.h` | Each ESP32 node |
| MQTT credentials | `firmware/access_node/secrets.h` | Each ESP32 node |
| 32-byte master key | `firmware/access_node/secrets.h` | Each ESP32 node |
| 32-byte master key | `apps/kiosk_station/secrets.py` | Python kiosk app |
| 32-byte master key | `apps/kiosk_station/station_writer/secrets_station.h` | Card writer firmware |
| Firebase service account | `services/rpi_bridge/service-account.json` | Bridge |
| Firebase env vars | `apps/web_app/.env` | Web app |

**The master key must be identical in all three locations. A mismatch means cards issued by the kiosk cannot be read by the nodes.**

---

## Emergency Contacts / Escalation

> Fill in before deployment:
> - Lead engineer: ___________________
> - Firebase project ID: ___________________
> - RPi static IP: ___________________
> - WiFi SSID: ___________________
