# Section 04 — Machine Registry

← [Bill of Materials](03_bill_of_materials.md) | → [Hardware Assembly](05_hardware_assembly.md)

---

## Why This Section Exists

The machine ID is the single most critical configuration value in MMS V2. It appears in three places simultaneously:
1. The ESP32 node firmware (`config.h` → `MACHINE_ID`)
2. The kiosk station Python app (`apps/apps/kiosk_station/config.py` → `MACHINE_IDS` dict)
3. Firestore (`/machines/{id}` document ID)

If these three are inconsistent — even by one number — cards issued for "Soldering Station 1" will be denied on the node claiming to be "Soldering Station 1" (because the permission bit will be wrong), and the node will log events under the wrong machine ID in Firestore.

**Read this section before flashing any node or issuing any card.**

---

## Canonical Machine Registry (Current Deployment)

| Machine ID | Machine Name | Location | Relay Type | Session Limit | Permission Bit | Firestore Doc ID |
|---|---|---|---|---|---|---|
| 1 | Creality 3D Printer | Zone A | Mechanical (active HIGH) | 60 min | Bit 0 | `"1"` |
| 2 | Fracktal Laser Cutter | Zone B | Mechanical (active HIGH) | 30 min | Bit 1 | `"2"` |
| 3 | Soldering Station 1 | Zone C | Mechanical (active HIGH) | 15 min | Bit 2 | `"3"` |
| 4 | Soldering Station 2 | Zone C | Mechanical (active HIGH) | 15 min | Bit 3 | `"4"` |
| 5 | Bosche Grinder | Zone D | Mechanical (active HIGH) | 20 min | Bit 4 | `"5"` |

**Permission bitmask examples:**
- Student allowed on all 5 machines: `0b00011111` = `0x1F` = `31` decimal
- Student allowed only on Soldering Stations (3 and 4): `0b00001100` = `0x0C` = `12` decimal
- Student allowed on 3D Printer only: `0b00000001` = `0x01` = `1` decimal

**Formula:** `perm_mask = sum(1 << (machine_id - 1) for each allowed machine_id)`

---

## The Three Places To Keep In Sync

### Place 1: `firmware/access_node/config.h`
```cpp
#define MACHINE_ID    3
#define MACHINE_NAME  "Soldering Station 1"
```
This is flashed into one specific physical node. Every node gets its own flash with its own MACHINE_ID.

### Place 2: `apps/apps/kiosk_station/config.py`
```python
MACHINE_IDS = {
    "Creality 3D Printer":    1,
    "Fracktal Laser Cutter":  2,
    "Soldering Station 1":    3,
    "Soldering Station 2":    4,
    "Bosche Grinder":         5,
}
```
This is used by the kiosk to build the permission bitmask when issuing a card. The kiosk UI shows a machine name dropdown — the admin selects which machines the student is allowed to use, and the kiosk derives the bitmask.

### Place 3: Firestore `/machines/{id}`
```json
{
  "name": "Soldering Station 1",
  "session_limit_minutes": 15,
  "relay_active_high": true,
  "machine_active": true,
  "location": "Zone C"
}
```
Document ID is the string `"3"`. This is seeded by `scripts/seed_machines.py`.

---

## Procedure: Adding A New Machine

Follow all 6 steps. Do not skip any.

### Step 1 — Assign the next available ID

Check the current registry above. The next available ID is the highest current ID + 1. Never reuse a retired ID — cards already issued with a retired ID's bit set will unexpectedly gain access to the new machine.

*Example: If 5 machines exist (IDs 1–5), the next machine is ID 6.*

### Step 2 — Add to kiosk config

Edit `apps/apps/kiosk_station/config.py`:
```python
MACHINE_IDS = {
    ...
    "New Machine Name":  6,   ← add this line
}
```
Restart the Flask kiosk app after editing.

### Step 3 — Add to Firestore

Either use the web admin dashboard (Machines → Add Machine) or run:
```bash
# Add to seed_machines.py MACHINES list, then run:
python3 scripts/seed_machines.py
```

Or manually via Firebase Console:
```
Firestore → machines → Add document
  Document ID: "6"
  Fields:
    name: "New Machine Name"
    session_limit_minutes: 15
    relay_active_high: true
    machine_active: true
    location: "Zone X"
```

### Step 4 — Flash the new node

In `firmware/access_node/config.h`:
```cpp
#define MACHINE_ID    6
#define MACHINE_NAME  "New Machine Name"
```

Flash the firmware. See [Section 11 — Node Provisioning](11_access_node_provisioning.md).

### Step 5 — Restart the bridge

The bridge's config listener will detect the new Firestore document and push the config to the new node via MQTT.
```bash
ssh pi@192.168.0.10
sudo systemctl restart mms-bridge
```

### Step 6 — Update cards for users who need access

Existing cards do not automatically gain access to the new machine. Each user who needs access to Machine 6 must have their card re-issued with the new bitmask. Go to the kiosk, re-issue the card with Machine 6 ticked. The old bitmask is overwritten.

---

## Procedure: Renaming A Machine

Renaming does not require card re-issue — permissions are stored as a bitmask, not a name.

### Step 1 — Update Firestore

In Firebase Console or web app admin:
```
Firestore → machines → {id} → edit "name" field
```

### Step 2 — Update `apps/apps/kiosk_station/config.py`

```python
MACHINE_IDS = {
    ...
    "New Name":  3,   ← change "Soldering Station 1" to "New Name"
}
```

### Step 3 — Restart kiosk app

The kiosk dropdown will now show the new name.

### Step 4 — Push config to node

The bridge's Firestore config listener will detect the name change and push `{name: "New Name"}` to `mms/nodes/3/config` (retained). The node will update its NVS and OLED display within seconds.

**Verification:** Check the 128×64 OLED on the node — it should show the new machine name within 10 seconds of the Firestore change.

---

## Procedure: Decommissioning A Machine

Decommissioning a machine means disabling it so it can't be used, but keeping its ID reserved.

### Step 1 — Disable in Firestore

In Firebase Console:
```
Firestore → machines → {id} → set machine_active: false
```

The bridge will push `{machine_active: false}` to the node. The node will transition to MACHINE_DISABLED state — all card taps will be denied, OLED will show "Machine Offline".

### Step 2 — Remove from kiosk dropdown

Remove the line from `apps/apps/kiosk_station/config.py`. This prevents the kiosk admin from accidentally granting permissions to the decommissioned machine.

### Step 3 — Do NOT reuse the ID

Mark the ID as retired in the registry above (add "(RETIRED)" to the name in Firestore). If the machine is replaced by a different machine, give the new machine a new ID. Reusing IDs invalidates existing cards that have the old machine's bit set.

---

## Registry Change Log

Maintain this log. Date every change.

| Date | Change | Performed By |
|---|---|---|
| _YYYY-MM-DD_ | Initial seed — 5 machines | _Name_ |
| | | |
