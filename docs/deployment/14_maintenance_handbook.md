# Section 14 — Maintenance Handbook

← [Production Rollout](13_production_rollout.md) | → [Troubleshooting Guide](15_troubleshooting_guide.md)

---

## Overview

This section covers routine maintenance: daily checks, weekly tasks, monthly tasks, firmware updates, and database housekeeping. Following this schedule prevents most preventable failures.

---

## Daily Checks (5 minutes)

**Who:** Any lab admin with web app access.

```
1. Open web dashboard
2. Check all 5 machines show status (idle or active — not "offline" or "error")
3. Look for any session stuck in "active" state for more than 2 hours
   → If found: Remote Stop → check the physical machine
4. Check that RTDB /mms/bridge/status.state = "online"
   → If "offline": check RPi (see Troubleshooting Section 15)
```

If everything looks normal, daily check is done in under 2 minutes.

---

## Weekly Tasks (30 minutes)

**Who:** Lab admin or coordinator.

### 1. Check RPi health

```bash
ssh pi@192.168.0.10

# System resources
df -h /         # Disk usage — alert if > 80%
free -h         # RAM — alert if available < 100MB
uptime          # Load average — alert if 15-min avg > 2.0

# Bridge service health
sudo systemctl status mms-bridge
# Should be: Active: active (running)

# Bridge log — look for repeated errors
sudo journalctl -u mms-bridge --since "7 days ago" | grep -i "error\|exception\|failed" | tail -30
```

### 2. Rotate LittleFS logs (automatic — verify)

LittleFS logs rotate automatically at 50KB. But confirm no node has stale logs:
```
Check Serial Monitor on each node after a restart:
"[FS] sessions.jsonl: 0 bytes (flushed)"  ← good
"[FS] sessions.jsonl: 48000 bytes"        ← near rotation, will auto-rotate
"[FS] sessions.old.jsonl exists"          ← previous rotation preserved
```

If a node shows `sessions.jsonl` size growing indefinitely and never flushing, the MQTT connection may be broken silently — check that node's WiFi status.

### 3. Verify card stock

Count remaining blank MIFARE Classic 1K cards. Reorder when fewer than 20 remain. Cards needed: approximately 2 per new student enrolled.

### 4. Check Firestore index status

```bash
# From your laptop with Firebase CLI:
firebase firestore:indexes
# All 3 composite indexes should show status "Enabled"
# "Building" or "Error" status needs attention
```

---

## Monthly Tasks (1 hour)

### 1. Backup RPi SD card

```bash
# On your laptop with SD card reader:
# Shut down RPi first: ssh pi@192.168.0.10 "sudo shutdown now"
# Wait 30 seconds, then remove SD card

# Linux:
sudo dd if=/dev/sdX of=mms-bridge-$(date +%Y%m%d).img bs=4M status=progress

# Or use Raspberry Pi Imager → "Read" option

# Store backup on a separate drive or encrypted cloud storage
# Keep last 3 monthly backups; delete older ones
```

### 2. Check Firestore quota usage

- Firebase Console → Project Settings → Usage and billing
- Firestore reads/writes/deletes — should be well within Spark free tier limits
- At 15 machines × 30 sessions/day = 450 writes/day → 13,500 writes/month → far below Spark's 50,000/day

### 3. Archive old sessions (optional, after 6 months)

If the sessions collection grows large (>10,000 documents), sessions older than 3 months can be archived:

```python
# On your laptop:
from firebase_admin import credentials, firestore, initialize_app
import datetime

cred = credentials.Certificate("service-account.json")
initialize_app(cred)
db = firestore.client()

cutoff = datetime.datetime(2024, 1, 1)  # adjust date

old_sessions = db.collection("sessions").where("start_time", "<", cutoff).stream()
for doc in old_sessions:
    # Export to local JSONL file before deleting
    with open("archive_sessions.jsonl", "a") as f:
        f.write(doc.to_dict().__str__() + "\n")
    doc.reference.delete()
    print(f"Archived: {doc.id}")
```

### 4. Review and rotate MQTT password (if authentication enabled)

If MQTT authentication was enabled post-deployment (per Section 07):
- Rotate the `bridge` MQTT password every 3 months
- Update `/home/pi/mms/.env` MQTT_PASS
- Regenerate Mosquitto password file: `sudo mosquitto_passwd /etc/mosquitto/passwd bridge`
- Restart Mosquitto: `sudo systemctl restart mosquitto`
- Restart bridge: `sudo systemctl restart mms-bridge`

---

## Firmware Update Procedure

This is the standard OTA update path. Use it whenever a new firmware version is ready.

### Step 1 — Build the firmware binary

In Arduino IDE with the production access_node project open:
1. Sketch → Export Compiled Binary
2. Find the `.bin` file in the sketch folder (e.g., `access_node.ino.esp32.bin`)
3. Note the version number — update `#define FW_VERSION "2.x.x"` in `config.h` before building

### Step 2 — Upload to Firebase Storage

1. Firebase Console → Storage → `firmware/` folder
2. Upload the `.bin` file, naming it by version: `firmware/2.1.0.bin`
3. Click the uploaded file → copy the **download URL** (it is a long URL starting with `https://firebasestorage.googleapis.com/...`)

### Step 3 — Trigger OTA via RTDB

In Firebase Console → Realtime Database:
```
Set /mms/ota to:
{
  "firmware_url": "https://firebasestorage.googleapis.com/.../firmware/2.1.0.bin",
  "target_version": "2.1.0",
  "released_at": <current unix timestamp>
}
```

Or via web app: Super Admin Panel → OTA Update → paste URL and version → Submit.

### Step 4 — Monitor rollout

Each node will:
1. Receive OTA command from bridge (MQTT)
2. Transition to OTA state: OLED shows "Updating FW..."
3. Download firmware from URL (~45s on typical WiFi)
4. Reboot to new firmware
5. Report new version in RTDB `/mms/nodes/{id}/firmware_version`

Monitor on dashboard:
```
Node 1: updating... → idle (v2.1.0)  ✓
Node 2: updating... → idle (v2.1.0)  ✓
...
```

If a node shows `error` state after OTA:
- It rebooted but firmware has a critical bug
- Firmware's rollback mechanism activates after 3 crashes
- Node reverts to previous version automatically
- Check serial output for specific error

### Step 5 — Verify OTA success

After all nodes report the new version:
1. Run integration test (partial — just Tests 1, 4, 5) on one node
2. Confirm session logging still works with new firmware
3. Update the firmware version in deployment documentation

---

## Database Maintenance

### Cleaning up orphaned sessions

An "orphaned active session" is a Firestore session document with `status:"active"` but no corresponding activity on the node. This happens during power loss or network errors.

**Detect:**
```javascript
// Firebase Console → Firestore → sessions → filter: status == "active"
// Any session with start_time > 3 hours ago is orphaned
```

**Fix (via Firebase Console):**
- Find the document
- Set `status` to `"completed"`, set `end_time` to current timestamp
- Set `end_reason` to `"power_loss"`

**Automated cleanup script (run weekly via cron on RPi):**
```python
# /home/pi/mms/cleanup_orphans.py
from firebase_admin import credentials, firestore, initialize_app
import datetime

cred = credentials.Certificate("/home/pi/mms/service-account.json")
initialize_app(cred)
db = firestore.client()

cutoff = datetime.datetime.utcnow() - datetime.timedelta(hours=3)

orphans = (
    db.collection("sessions")
    .where("status", "==", "active")
    .where("start_time", "<", cutoff)
    .stream()
)

for doc in orphans:
    doc.reference.update({
        "status": "completed",
        "end_time": firestore.SERVER_TIMESTAMP,
        "end_reason": "power_loss",
    })
    print(f"Cleaned orphan: {doc.id}")
```

Add to RPi crontab:
```bash
crontab -e
# Add:
0 3 * * * /home/pi/mms/venv/bin/python3 /home/pi/mms/cleanup_orphans.py >> /var/log/mms-cleanup.log 2>&1
```

### Cleaning up stale RTDB commands

If a command sits in RTDB `/mms/commands/{id}` unacknowledged for more than 10 minutes, the bridge should auto-clean it. If it does not (bridge was down), clean manually:
```
RTDB Console → /mms/commands → delete the specific machine's command node
```

---

## Log Management

### RPi journal log rotation

systemd journal rotates logs automatically. Verify rotation is configured:
```bash
sudo journalctl --disk-usage
# Typical: Archived and active journals take up 200.0M in /var/log/journal/
# This is fine. systemd limits journal to 10% of filesystem by default.
```

### Mosquitto log rotation

```bash
# Mosquitto logs to syslog. Check:
sudo grep mosquitto /var/log/syslog | tail -20
# Or
sudo journalctl -u mosquitto --since "1 day ago" | tail -30
```

---

## Summary Schedule

| Task | Frequency | Time | Who |
|---|---|---|---|
| Dashboard health check | Daily | 2 min | Admin |
| RPi health check | Weekly | 10 min | Admin |
| Firestore index check | Weekly | 2 min | Admin |
| Card stock count | Weekly | 2 min | Coordinator |
| RPi SD card backup | Monthly | 30 min | Admin |
| Firestore quota review | Monthly | 5 min | Admin |
| Orphan session cleanup (manual check) | Monthly | 10 min | Admin |
| MQTT password rotation | Quarterly | 15 min | Admin |
| Firmware update (as needed) | Per release | 30 min | Admin |
