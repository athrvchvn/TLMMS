# Section 12 — Integration Testing

← [Access Node Provisioning](11_access_node_provisioning.md) | → [Production Rollout](13_production_rollout.md)

---

## Purpose

Integration testing confirms that all system components (ESP32 node → MQTT → RPi bridge → Firebase → web dashboard) work together end-to-end. Run this before deploying any node to a live machine.

**Required:** At least one provisioned node (Section 11), RPi running bridge.py, Firebase deployed, web app deployed, at least one issued card.

---

## Test Environment Setup

```
Laptop (web browser, Serial Monitor)
    │
    ├── USB → ESP32 Node (provisioned, connected to lab WiFi)
    │
    └── LAN → Raspberry Pi (192.168.0.10)
              ├── Mosquitto broker
              └── bridge.py → Firebase
                               ├── Firestore
                               └── RTDB
```

**Before starting:** Open all of these simultaneously:
1. Serial Monitor at 115200 baud on the node
2. Firebase Console → Firestore → sessions (real-time view)
3. Firebase Console → RTDB → mms/nodes/{your machine ID}
4. Web app dashboard in browser (`https://your-project.web.app`)

---

## Test 1 — Normal Session (Card Tap + Remove)

**Expected outcome:** Session recorded in Firestore; RTDB shows active state during session; dashboard shows live status.

**Steps:**
1. Insert a permitted card into the HC89 slot
2. Observe:
   - Serial: `[ACCESS] GRANTED` → `[RELAY] ON` → `[SESSION] Started`
   - RTDB: `/mms/nodes/1` state changes to `"active"` within 2s
   - Dashboard: machine card shows student's roll number and session timer
3. Wait 30 seconds
4. Remove card
5. Observe:
   - Serial: `[HC89] Card absent` → `[RELAY] OFF` → `[SESSION] Ended`
   - RTDB: `/mms/nodes/1` state changes to `"idle"` within 2s
   - Firestore `/sessions`: new document with `start_time`, `end_time`, `duration_s`, `end_reason:"card_removed"`
   - Dashboard: machine card returns to idle state

**Pass criteria:**
- [ ] Relay energised within 1s of card detection
- [ ] RTDB status updates within 2s
- [ ] Firestore session document created with correct roll number
- [ ] `end_reason` = `"card_removed"`
- [ ] Dashboard updates without manual refresh

---

## Test 2 — Access Denial (Non-permitted Card)

**Steps:**
1. Insert a card that does NOT have permission for this machine
2. Observe:
   - Serial: `[ACCESS] DENIED — permission bit = 0`
   - OLED: "Access Denied" with reason
   - LED: 3 quick red flashes
   - Relay: stays OFF (confirm with multimeter or by checking the machine does not power on)
3. After 3 seconds, node returns to IDLE automatically

**Pass criteria:**
- [ ] Relay never energises
- [ ] No session document created in Firestore
- [ ] OLED shows denial reason

---

## Test 3 — Session Timeout

**Steps:**
1. Temporarily change session limit to 1 minute:
   - Web app → Machine Management → select machine → Edit → Session Limit: 1 min → Save
   - Confirm RTDB/MQTT config pushed (serial: `[MQTT] Received config: {session_limit:1...}`)
2. Insert permitted card — start session
3. Wait 58 seconds (2 minutes warning — this is 2 min before limit, but at 1 min limit the warning at `limit - 2min` = negative, so it skips and goes directly to timeout)
4. Wait for 60 seconds total
5. Observe: relay turns OFF, OLED shows "Session Expired", Firestore `end_reason:"timeout"`
6. Restore session limit to original value

**Pass criteria:**
- [ ] Relay de-energises at timeout
- [ ] `end_reason` = `"timeout"` in Firestore
- [ ] Node returns to IDLE and accepts next card

---

## Test 4 — Remote Stop

**Steps:**
1. Start a session by inserting a permitted card
2. Confirm dashboard shows the active session
3. In web dashboard: click "Stop Session" on this machine's card
4. Observe:
   - RTDB: `/mms/commands/1` written immediately
   - Serial: `[MQTT] Received command: stop`
   - Relay: OFF within 2s
   - Serial: `[SESSION] Ended — reason: remote_stop`
   - Firestore: session document created with `end_reason:"remote_stop"`
   - RTDB: `/mms/commands/1` shows `acknowledged: true`

**Pass criteria:**
- [ ] Relay OFF within 2 seconds of clicking Stop in dashboard
- [ ] `end_reason` = `"remote_stop"` in Firestore
- [ ] RTDB command acknowledged flag set to true

---

## Test 5 — Card Revocation

**Steps:**
1. Note the UID of the card you are using (visible on serial: `[RFID] UID: A1B2C3D4`)
2. In web app: User Management → select the user → Revoke Card → Confirm
3. Observe bridge log: `[BRIDGE] Revocation list updated — 1 UID(s) revoked`
4. Observe node serial: `[MQTT] Received revoked list — 1 UID(s)` → `[FS] Revocation list updated`
5. Insert the revoked card
6. Observe: `[ACCESS] DENIED — card revoked`
7. OLED shows "Card Revoked"

**Pass criteria:**
- [ ] Denial happens within 5 seconds of revocation in web app
- [ ] Revoked card denied even if permissions would otherwise allow access
- [ ] Relay never energises

---

## Test 6 — Offline Session (RPi Unavailable)

**Steps:**
1. Stop the bridge: `sudo systemctl stop mms-bridge` on RPi
2. Also stop Mosquitto: `sudo systemctl stop mosquitto` on RPi
3. Wait for node serial to show: `[MQTT] Connection failed — operating offline`
4. Insert a permitted card
5. Session should be granted (offline access decision is local)
6. Remove card after 30s
7. Observe: session written to LittleFS (`[FS] Log appended`)
8. Restart Mosquitto: `sudo systemctl start mosquitto`
9. Restart bridge: `sudo systemctl start mms-bridge`
10. Watch node serial for reconnect and flush:
```
[MQTT] Connected
[FS] Flushing 1 events from LittleFS
[MQTT] Published event: {t:..., r:"220002123", m:1, ...}
[FS] Log cleared after successful flush
```
11. Confirm session appeared in Firestore

**Pass criteria:**
- [ ] Access granted while offline
- [ ] Session correctly stored in LittleFS during outage
- [ ] Session appears in Firestore after reconnection
- [ ] No duplicate Firestore documents

---

## Test 7 — Stale Command (5-minute TTL)

**Steps:**
1. Stop Mosquitto on RPi: `sudo systemctl stop mosquitto`
2. In web dashboard: click "Stop Session" on an active-looking machine (even if no session is running)
   - This writes a command to RTDB with current timestamp
3. Wait 6 minutes
4. Restart Mosquitto and bridge: `sudo systemctl start mosquitto && sudo systemctl start mms-bridge`
5. Node reconnects to MQTT
6. Observe serial:
```
[MQTT] Received command: stop
[CMD] Command timestamp is 360s old (> 300s TTL) — discarding
[MQTT] Published ack: {acknowledged: true, ack_at: ...}
```
7. Relay should NOT toggle — the command was discarded as stale

**Pass criteria:**
- [ ] Stale command discarded without executing
- [ ] Command acknowledged in RTDB (so it doesn't repeat)
- [ ] Relay state unchanged

---

## Test 8 — Config Push (Live Update)

**Steps:**
1. Observe current session limit in OLED or serial
2. In web app: Machine Management → select machine → Edit → change Session Limit (e.g., 45 → 30 minutes)
3. Within 5 seconds, observe node serial:
```
[MQTT] Received config: {session_limit:30, ...}
[NVS] Config updated: session_limit = 30
```
4. Start a session and confirm the new limit is shown on OLED

**Pass criteria:**
- [ ] Config change reflected on node within 5s
- [ ] NVS updated (persists across reboot)
- [ ] New limit shown on OLED during active session

---

## Test 9 — Watchdog Recovery

**Only run this on a test bench, not on a deployed node.**

**Steps:**
1. Flash a modified firmware that deliberately blocks the main loop for 35 seconds (longer than 30s TWDT)
2. Boot the node
3. After ~30 seconds: node should reboot automatically
4. Observe on serial (after reboot):
```
[SYS] Boot reason: WDT reset
[SYS] Previous session may have been active — checking LittleFS
```

**Pass criteria:**
- [ ] Node reboots within 35 seconds of blocking the loop
- [ ] Node recovers to IDLE after reboot
- [ ] Boot reason logged as WDT

---

## Test 10 — Duplicate Session Prevention

**Steps:**
1. Flash the same offline session log to LittleFS twice (copy the same event twice in sessions.jsonl using a test sketch)
2. Reconnect to MQTT — both events flush
3. Check Firestore — should only show ONE session document

**Mechanism to verify:** Bridge.py checks for existing session within ±5s before inserting. Second event matches → skipped.

**Pass criteria:**
- [ ] Exactly 1 Firestore document despite 2 identical events being flushed

---

## Integration Test Sign-off Sheet

```
Integration Test Sign-off
Machine ID:  [ ]  Name: _______________________
Date: __________  Engineer: ___________________
RPi IP: ____________  Firebase Project: _________

Test 1  — Normal Session        [ ] PASS  [ ] FAIL
Test 2  — Access Denial         [ ] PASS  [ ] FAIL
Test 3  — Session Timeout       [ ] PASS  [ ] FAIL
Test 4  — Remote Stop           [ ] PASS  [ ] FAIL
Test 5  — Card Revocation       [ ] PASS  [ ] FAIL
Test 6  — Offline Session       [ ] PASS  [ ] FAIL
Test 7  — Stale Command TTL     [ ] PASS  [ ] FAIL
Test 8  — Config Push           [ ] PASS  [ ] FAIL
Test 9  — Watchdog Recovery     [ ] SKIP (bench only) [ ] PASS [ ] FAIL
Test 10 — Dedup Prevention      [ ] SKIP (optional)  [ ] PASS [ ] FAIL

Overall result:  [ ] PASS  [ ] FAIL
Notes: _______________________________________________

Sign-off: _______________________________
```

**A node passes integration testing if Tests 1–8 all pass.**
Tests 9 and 10 are optional during initial rollout but should be done before full production deployment.

Proceed to [Section 13 — Production Rollout](13_production_rollout.md).
