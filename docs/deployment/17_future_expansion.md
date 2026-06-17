# Section 17 — Future Expansion

← [Disaster Recovery](16_disaster_recovery.md)

---

## Overview

This section documents planned expansion paths for MMS V2. Each expansion is designed to work within the existing architecture. None require redesigning the core system.

---

## Adding a New Machine

The permission bitmask supports 32 machines. Adding machines 6–32 requires:

### 1. Update the Machine Registry in three places

See [Section 04 — Machine Registry](04_machine_registry.md) for the three-places-in-sync rule.

**`firmware/access_node/config.h`** — add to the canonical table comment (firmware read reference only; nodes use their own MACHINE_ID):
```cpp
// Machine 6: Band Saw, 25min, bit 5, Zone E
```

**`services/rpi_bridge/bridge.py` or `config.py`** — if bridge has a machine name map, add the entry:
```python
MACHINE_NAMES = {
    ...
    6: "Band Saw",
}
```

**`v2/scripts/seed_machines.py`** — add the new machine to the seed script:
```python
machines = [
    # ... existing 5
    {
        "id": "6",
        "name": "Band Saw",
        "session_limit_minutes": 25,
        "relay_active_high": True,
        "machine_active": True,
        "location": "Zone E",
    },
]
```

Then run:
```bash
python3 scripts/seed_machines.py
# Output: [6] Band Saw — CREATED
```

### 2. Assemble, validate, and provision the new node

Follow Sections 05, 06, 11.

### 3. Update user permissions

When issuing cards to users who need access to the new machine:
- Web app → User Management → edit user → enable the new machine checkbox
- Re-issue the card (permission change requires card re-issue)

Users who should not have access to the new machine need no change — their bitmask bit 5 is already 0.

---

## PIN Entry at Machine (Phase 4)

The rotary encoder (EC11) wiring is already included in the hardware assembly (CLK=GPIO34, DT=GPIO35, SW=GPIO25). The access node firmware has a dedicated module slot for `pin_verifier.h/.cpp`.

The card already stores the PIN hash in Block 5 bytes 0–7. No card re-issue needed.

### What needs to be built

1. **Rotary encoder input handler** — CLK/DT interrupts for rotation, SW for button. Already tested in hardware validation Test 09.

2. **PIN entry UI on 128×64 OLED:**
   - Show `Enter PIN: ____`
   - Rotate encoder to select digit (0–9)
   - Press SW to confirm digit
   - After 4 digits: compute `HMAC-SHA256(master_key ∥ uid_hex, entered_pin)[0:7]`
   - Compare with block 5 bytes 0–7
   - If match: grant access
   - If no match after 3 tries: deny, 30s lockout

3. **New firmware state `PIN_ENTRY`:**
   - Inserted after CARD_READING succeeds (permissions check passes)
   - If PIN correct → SESSION_ACTIVE
   - If PIN wrong 3× → ACCESS_DENIED with reason "wrong_pin"

4. **Timeout:** PIN entry window = 30 seconds, then timeout → IDLE

---

## Multi-Lab Deployment

To deploy MMS to a second lab (e.g., a different department or campus):

### Option A — Same Firebase, Different MQTT Network

- New RPi + Mosquitto in the second lab
- New bridge.py instance connecting to the same Firebase project
- Bridge uses `lab_id` prefix on MQTT topics: `mms/lab2/nodes/{id}/...`
- Firestore uses separate machine IDs (e.g., 11–20 for second lab)
- Same web app; admin sees all machines

**What to change:**
- `config.h`: `MACHINE_ID` must be globally unique across all labs
- `bridge.py`: configure with lab-specific MQTT topic prefix
- Seed machines with IDs 11–20 (or however many are in lab 2)

### Option B — Separate Firebase Project per Lab

- Each lab has its own Firebase project
- Each lab has its own web app deployment
- Cards issued by lab 1 kiosk are not valid in lab 2 (different master key per lab)

**Use this when:** Labs are under separate administration or must keep data strictly isolated.

---

## Machine Reservation / Booking System

This is a separate system that would sit alongside MMS but connect to the same Firestore. Basic design:

```
Reservation Firestore schema:
/reservations/{reservation_id}
  roll_number:    string
  machine_id:     string
  start_time:     timestamp (requested slot start)
  end_time:       timestamp (requested slot end)
  status:         "confirmed" | "cancelled" | "completed"
  created_at:     timestamp
```

**MMS integration point:** When a card is tapped, check if there is an active reservation for that roll number on that machine at the current time. If yes: grant. If no active reservation and machine is busy: deny with "machine reserved by another user."

This requires an internet-connected access decision (not offline-capable). Design carefully for offline fallback.

---

## Analytics Dashboard

Firestore `/sessions` contains all the data needed for analytics. Since the web app is already connected to Firestore, adding an analytics page requires only frontend work:

**Useful queries (already possible with existing indexes):**
- Sessions per machine per day (index: machine_id + start_time)
- Sessions per user per month (index: roll_number + start_time)
- Average session duration per machine
- Peak usage hours (group by start_time hour)
- Most frequent users
- Machines with highest timeout rate

**Implementation:** Add a `/analytics` route to the React web app. Use Firestore aggregation queries or client-side reduce on a reasonable date range.

---

## Multi-Factor Authentication (Future Security Upgrade)

Currently: card tap = access (card carries permissions).

Future option: card tap + PIN (Phase 4 above) + phone confirmation (MFA).

**Phone confirmation flow:**
1. Card tapped → permissions verified → node publishes event to MQTT
2. Bridge receives → looks up user's phone number in Firestore
3. Bridge sends SMS via Twilio (or similar) with a 6-digit OTP
4. User enters OTP on keypad (rotary encoder or numeric)
5. Bridge verifies OTP → sends "confirm" MQTT message to node
6. Node receives confirmation → grants access

This adds a cloud-round-trip to the access path — means it cannot work offline. Only suitable if the lab always has reliable connectivity.

---

## RFID Card Renewal / Expiry

Block 6 (currently reserved/all-zeros) can be used to store an expiry date:

```
Block 6 — Card Validity (16 bytes):
[0..3]   Expiry timestamp (uint32 LE, Unix time)
[4]      Academic year (e.g., 0x27 = 2027)
[5..15]  Reserved
```

No firmware changes needed except adding expiry check in `card_schema.cpp`:
```cpp
if (data.expiry_ts > 0 && now > data.expiry_ts) {
    return ACCESS_DENIED_EXPIRED;
}
```

Cards would expire at the end of each academic year and need renewal (re-issue with new expiry date). No permission changes needed for renewal — just re-write Block 6.

---

## WiFi Mesh / Long-Range Coverage

For labs where some machines are too far from the main WiFi AP:

**Option A — Add 2.4GHz access point**
Simple and effective. Set the same SSID and password — ESP32 nodes roam automatically.

**Option B — Ethernet backhaul + distributed AP**
For very large labs: Ethernet to AP near each machine cluster. Same SSID.

**Option C — ESP-NOW backup link**
For machines with no WiFi (buried in faraday cages, RF shielding):
- One "gateway" ESP32 has WiFi + ESP-NOW
- Shielded ESP32 nodes use ESP-NOW to communicate with gateway
- Gateway bridges ESP-NOW ↔ MQTT
- Significant firmware complexity; only worth it if WiFi is genuinely impossible

---

## Summary

| Expansion | Difficulty | Card Re-issue | Firmware Change | Firebase Change |
|---|---|---|---|---|
| Add machine (6th+) | Easy | For new permissions only | No | Add to Firestore |
| PIN entry at machine | Medium | No | Yes (new state + UI) | No |
| Multi-lab deployment | Medium | Per lab | Minor | Option A: No |
| Reservation system | Hard | No | Yes | Yes (new collection) |
| Analytics | Easy | No | No | No |
| MFA (phone OTP) | Hard | No | Yes (new state) | Yes (Twilio integration) |
| Card expiry | Easy | Yes (all cards) | Yes (1 check) | No |
