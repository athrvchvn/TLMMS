# Section 16 — Disaster Recovery

← [Troubleshooting Guide](15_troubleshooting_guide.md) | → [Future Expansion](17_future_expansion.md)

---

## Overview

Disaster recovery covers scenarios where routine troubleshooting is insufficient — hardware has failed, data may be lost, or core secrets are compromised. Each procedure below describes the worst-case scenario and the steps to recover.

---

## DR-1 — Raspberry Pi Failure

### Symptoms
- Bridge health endpoint unreachable (`curl http://192.168.0.10:8080/health` times out)
- All nodes show MQTT reconnect retries in serial
- Dashboard shows all machines offline

### Impact
- Nodes continue operating offline (access still granted/denied correctly)
- Sessions accumulate in LittleFS on each node
- No sessions reach Firestore until bridge recovers
- No remote stop or OTA during outage

### Recovery Procedure

**Fastest recovery: restore from SD card backup**

1. Get the most recent SD card backup image
2. Flash to a new SD card using Raspberry Pi Imager (or `dd`)
3. Insert into new/repaired RPi and boot
4. Verify IP is correct (`ip addr show eth0` — should be 192.168.0.10)
5. Check bridge started: `sudo systemctl status mms-bridge`
6. If IP is different, update DHCP reservation in router

**Expected time:** 20–40 minutes

**If no SD backup available:**

1. Flash fresh RPi OS Lite to new SD card
2. Follow Section 07 from the beginning (takes ~2 hours)
3. The `service-account.json` must be retrieved from the secure backup location
4. After bridge is running, nodes will reconnect and flush LittleFS buffers automatically

**After recovery:**
- Verify all nodes reconnect and flush their offline sessions
- Check Firestore for sessions that arrived after bridge came back
- Run integration Test 6 (offline sync) to confirm flush worked

---

## DR-2 — Firebase Outage (Google-side)

### Symptoms
- Bridge log: `[FIREBASE] connection error: UNAVAILABLE`
- Sessions not appearing in Firestore
- Dashboard loads but shows stale data

### Impact
- Nodes continue operating (local access decisions)
- Bridge accumulates unwritten events in memory (not on disk — lost if bridge restarts)
- Dashboard updates stop

### Recovery Procedure

1. Check Firebase status page for ongoing incident
2. **Nothing to do on your side** — Firebase will recover automatically
3. While Firebase is down: bridge will retry writes; up to 100 events may be in memory
4. After Firebase recovers: bridge resumes writing
5. Restart bridge once Firebase is confirmed healthy: `sudo systemctl restart mms-bridge`

**Events that may be lost:** If bridge restarts during an extended Firebase outage, any unwritten events in bridge memory are lost. These sessions will appear as gaps in Firestore logs. The node's LittleFS log is the source of truth — nodes re-flush on MQTT reconnect, so the bridge can re-receive events if the node is still powered.

**Long outage (>4 hours):**
- Nodes' LittleFS buffers may fill (50KB limit ~500 short sessions)
- After 50KB: LittleFS rotates, deleting the oldest events
- A 4+ hour outage for a busy lab could lose oldest sessions in the buffer

---

## DR-3 — ESP32 Node Hardware Failure

### Symptoms
- Node does not boot (no OLED output, no serial output)
- Node reboots in a loop (Section 15, Symptom 13)
- MFRC522 physically damaged (no card reads)
- Relay failed (machine never turns off / never turns on)

### Impact
- One machine is out of service
- Machine must be bypassed (connected directly to mains, bypassing relay)

### Recovery: Replace ESP32 Module

1. Bypass machine: disconnect relay load wires, reconnect machine directly to mains
2. Remove failed ESP32 from enclosure
3. Insert replacement ESP32 NodeMCU 38-pin
4. Flash with production firmware (Section 11, Steps 1–3)
5. Same `secrets.h` and `config.h` as the original
6. Reinstall in enclosure and reconnect all wiring
7. Run integration tests (Section 12)
8. Remove mains bypass

**Time to replace one node:** 45–60 minutes

### Recovery: Replace MFRC522

1. Power off node
2. Desolder or unclip MFRC522 module
3. Install new MFRC522, same wiring
4. Power on — run `02_rfid` test sketch to verify
5. Re-flash production firmware (module is not NVS — firmware unchanged)

### Recovery: Replace Relay

1. Power off node AND machine at wall switch
2. Disconnect relay load wires
3. Desolder relay module
4. Install same relay module model
5. Reconnect load wires (COM and NO)
6. Power on — test relay with multimeter before connecting machine
7. Verify `relay_active_high` config matches the replacement relay module

---

## DR-4 — Lost Master Key

### Symptoms
- `secrets.h` was not backed up
- secrets.py for kiosk was deleted
- The lab computer with the key was wiped

### Impact
- **All existing cards are permanently invalid** (cannot be authenticated)
- New key must be generated
- Every node must be re-flashed with new key
- Every card must be re-issued

### Recovery Procedure

1. Generate a new 32-byte master key (Section 10, Step 1)
2. Store the new key in at minimum 3 separate locations (password manager, printed copy, encrypted cloud backup)
3. Update `secrets.h` in all node firmware and re-flash all nodes (Section 11)
4. Update `secrets_station.h` in station writer and re-flash
5. Update `secrets.py` in kiosk app and restart
6. Re-issue cards for all users (Section 10, Card Issuance Workflow)
   - All existing cards will fail RFID auth → all students must get new cards
7. Old card UIDs may remain in the revoked list — they are harmless but can be cleaned up

**This is the worst operational scenario.** It requires physically re-issuing cards to potentially 300 students. Prevention:
- Store master key in a password manager (Bitwarden, 1Password, or similar)
- Print one copy and store in a sealed envelope in the lab safe

---

## DR-5 — Corrupted Firmware (All Nodes)

### Symptoms
- OTA update was pushed with a firmware binary that causes all nodes to crash-loop
- Automatic rollback triggers on each node (3 crash → rollback)
- If rollback fails on some nodes, they become unresponsive

### Recovery Procedure

**If rollback triggers successfully on all nodes:**
- Nodes automatically revert to the previous firmware partition
- Verify each node recovers: watch serial for "Rolled back to previous firmware"
- Do not push another OTA until the firmware bug is fixed

**If rollback fails on some nodes (manual recovery required):**

1. For each stuck node:
   a. Connect USB cable from laptop to ESP32
   b. In Arduino IDE: open the last known-good firmware sketch
   c. Upload via USB (this overwrites OTA partitions)
   d. Confirm boot success on serial
   e. Re-run integration Test 1 (normal session)

2. After all nodes are recovered:
   a. Fix the firmware bug
   b. Build new binary
   c. Upload to Firebase Storage as a new version
   d. Trigger OTA again (Section 14, Firmware Update Procedure)

**Prevention:**
- Always test OTA on one node at a bench first
- Keep a verified `.bin` file of the last known-good release in a safe location

---

## DR-6 — Lost Firebase Admin Access

### Symptoms
- The Firebase admin account (Google account that owns the project) was deleted or lost
- No one can log in to Firebase Console

### Recovery Procedure

**Option A — Another project owner exists:**
- Use their account to add a new IAM owner

**Option B — Using Google account recovery:**
- Try account recovery at accounts.google.com
- If the email address is an institutional account, contact the institution's IT administrator

**Option C — Recreate the project (worst case):**
If the Firebase project itself is deleted or inaccessible:
1. Create a new Firebase project
2. Follow Section 08 to set up all services
3. Re-deploy all security rules and indexes
4. Re-seed machines collection
5. Re-deploy web app with new Firebase config
6. Re-provision all nodes with new Firebase credentials (bridge service-account.json, RTDB URL)

**Data loss in Option C:** All historical sessions are lost. User accounts in Firestore are lost (but RFID cards are still valid — they contain the data, and the master key is still intact). The kiosk can re-issue cards as needed.

---

## DR-7 — Database Corruption (Firestore Data Issues)

### Symptoms
- User documents have wrong structure (missing fields)
- Sessions collection has thousands of duplicate entries
- Machine documents were accidentally deleted

### Recovery: Restore from Backup

Firestore does not have built-in point-in-time restore on the Spark (free) tier.

**For machines collection (always recoverable):**
```bash
python3 scripts/seed_machines.py  # re-seeds idempotently
```

**For users collection:**
This is the most critical data. If corrupted:
- Check if any card still works at a node — the card itself contains roll number and permissions (not Firestore-dependent)
- Re-create user Firestore documents from the card data
- At kiosk: "Read UID" → shows card UID and block data → manually re-create Firestore user doc

**For sessions collection:**
Sessions are logs — past data that is corrupted cannot be recovered without a backup. Focus on preventing future corruption:
- Never write directly to Firestore `/sessions` — only the bridge writes sessions
- Never enable `allow write: if true` on sessions in Firestore rules

**Export before it gets worse:**
```bash
# On a laptop with Firebase CLI:
firebase firestore:export gs://your-project.appspot.com/firestore-export
# Creates a backup in Storage (requires Blaze plan)
# Or for small databases:
firebase firestore:export ./local-firestore-backup
```

---

## Emergency Contact Template

Fill in this template for the lab and keep it printed near the RPi:

```
MMS V2 Emergency Contacts
==========================
Primary Admin:    ________________  Phone: ____________  Email: ____________
Backup Admin:     ________________  Phone: ____________  Email: ____________
Firebase Project: ________________  Owner account: _________________________
RPi Static IP:    192.168.0.10
RPi SSH user:     pi
RPi SSH password: [stored in password manager: ___________________________]
Master key backup location: _______________________________________________
Service account backup location: _________________________________________
Firebase Console URL: https://console.firebase.google.com/project/___________
Web app URL: https://_________________________________.web.app
GitHub repo: https://github.com/______________________________________
```
