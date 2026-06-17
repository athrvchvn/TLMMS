# Section 08 — Firebase Deployment

← [Raspberry Pi Deployment](07_raspberry_pi_deployment.md) | → [Web Application Deployment](09_web_application_deployment.md)

---

## Why This Section Exists

Firebase provides the cloud layer: permanent session storage (Firestore), real-time machine status (RTDB), and firmware hosting (Storage). This section sets up the Firebase project from scratch, deploys security rules, and verifies the schema.

---

## Step 1 — Create Firebase Project

1. Go to [https://console.firebase.google.com](https://console.firebase.google.com)
2. Click "Add project"
3. Project name: `mms-tinkerers-lab` (or similar)
4. Disable Google Analytics (not needed)
5. Click "Create project"

**Plan selection:**
- **Spark (free)**: 1GB Firestore, 10GB RTDB, 10GB Storage. Sufficient for ~300 users and 15 machines indefinitely.
- **Blaze (pay-as-you-go)**: Required only if you exceed free limits or need Firebase Storage for OTA binaries >10GB.

For initial deployment, Spark is fine. OTA firmware binaries are typically <1MB each.

---

## Step 2 — Enable Firebase Services

In Firebase Console:

### Firestore
1. Build → Firestore Database → Create database
2. Select location: `asia-south1` (Mumbai — lowest latency from India)
3. Start in **production mode** (we will deploy rules in Step 6)

### Realtime Database
1. Build → Realtime Database → Create database
2. Select location: `asia-southeast1` (Singapore) — or same region as Firestore
3. Start in **locked mode** (we will deploy rules)

### Storage
1. Build → Storage → Get started
2. Select same region as Firestore
3. Start in production mode

### Authentication
1. Build → Authentication → Get started
2. Sign-in method → Enable "Email/Password"
3. (No Google/GitHub/etc. needed)

---

## Step 3 — Install Firebase CLI

On your **laptop** (not the RPi):
```bash
# Install Node.js if not present (https://nodejs.org)
node --version   # should be 18+

# Install Firebase CLI
npm install -g firebase-tools

# Login
firebase login
# Opens browser — log in with the Google account that owns the Firebase project

# Verify
firebase projects:list
# Should show your project
```

---

## Step 4 — Initialise Firebase in Repository

```bash
cd /path/to/MMS/v2

# Init Firebase in the v2 directory
firebase init

# Selections:
# ◉ Firestore
# ◉ Realtime Database
# ◉ Hosting (for web app)
# ◉ Storage (for OTA binaries)
#
# Use existing project → select your project
# Firestore rules file: firebase/firestore.rules (already exists)
# Firestore indexes file: firebase/firestore.indexes.json (already exists)
# RTDB rules file: firebase/database.rules.json (already exists)
# Public directory for hosting: apps/web_app/dist
# Single-page app: yes
# GitHub Actions: no (for now)
```

This creates a `.firebaserc` file linking the directory to your project.

---

## Step 5 — Deploy Security Rules and Indexes

```bash
cd /path/to/MMS/v2

# Deploy Firestore rules
firebase deploy --only firestore:rules
# Expected output: ✔  Deployed Firestore rules.

# Deploy Firestore indexes
firebase deploy --only firestore:indexes
# Expected output: ✔  Deployed indexes.
# Note: index build may take 5–10 minutes after this.

# Deploy RTDB rules
firebase deploy --only database
# Expected output: ✔  Deployed database rules.
```

**Verify rules deployed:**
- Firestore Console → Rules tab → should show the content of `firestore.rules`
- RTDB Console → Rules tab → should show `database.rules.json` content

**Verify indexes built:**
- Firestore Console → Indexes tab → should show 3 composite indexes with status "Enabled" (initially "Building")

---

## Step 6 — Seed the Machines Collection

The bridge's config listener watches `/machines/*` and pushes configs to nodes. This collection must be populated before any node connects.

```bash
# Set the service account credential
export GOOGLE_APPLICATION_CREDENTIALS=/path/to/service-account.json

# Preview what will be created
python3 scripts/seed_machines.py --dry-run

# Apply
python3 scripts/seed_machines.py
```

Expected output:
```
Seeding 5 machines...
  [1] Creality 3D Printer          — CREATED
  [2] Fracktal Laser Cutter        — CREATED
  [3] Soldering Station 1          — CREATED
  [4] Soldering Station 2          — CREATED
  [5] Bosche Grinder               — CREATED

Created: 5   Skipped: 0

Next steps:
  1. Restart rpi_bridge — it will push config to nodes via MQTT
  2. Flash each node with the matching MACHINE_ID
  3. Issue cards via the kiosk
```

**Verify in Firebase Console:**
- Firestore → machines → should show 5 documents: "1" through "5"

---

## Step 7 — Storage Configuration for OTA

OTA firmware binaries are uploaded to Firebase Storage and served via signed download URLs.

**Create the firmware folder:**
1. Storage Console → Files tab → New folder → `firmware`

**Upload firmware binary (when needed):**
1. Build the firmware in Arduino IDE: Sketch → Export Compiled Binary
2. This creates a `.bin` file
3. Upload to Storage → `firmware/2.1.0.bin` (version number in filename)
4. Click the uploaded file → Copy download URL

The download URL is the `firmware_url` written to RTDB when triggering OTA. See [Section 14 — Maintenance](14_maintenance_handbook.md) for OTA update procedure.

**Storage security rules (optional but recommended):**
```
rules_version = '2';
service firebase.storage {
  match /b/{bucket}/o {
    match /firmware/{allPaths=**} {
      allow read: if request.auth != null;
      allow write: if false;  // Only Admin SDK can write
    }
  }
}
```

---

## Firebase Schema Reference

### Firestore Collections

#### `/users/{roll_number}`
```json
{
  "roll_number":         "220002123",
  "name":                "Student Name",
  "email":               "student@iiti.ac.in",
  "pin_hash":            "aabbccdd11223344",
  "card_uid":            "A1B2C3D4",
  "machine_permissions": 7,
  "issued_at":           "2024-06-01T10:00:00Z",
  "issued_by":           "admin_uid",
  "is_active":           true,
  "last_seen":           "2024-06-15T14:30:00Z"
}
```

#### `/machines/{machine_id}` (document ID = "1", "2", ...)
```json
{
  "name":                  "Soldering Station 1",
  "session_limit_minutes": 15,
  "relay_active_high":     true,
  "machine_active":        true,
  "location":              "Zone C",
  "created_at":            "2024-06-01T00:00:00Z"
}
```

#### `/sessions/{session_id}` (auto-generated ID)
```json
{
  "machine_id":   "3",
  "machine_name": "Soldering Station 1",
  "roll_number":  "220002123",
  "user_name":    "Student Name",
  "start_time":   "2024-06-15T10:00:00Z",
  "end_time":     "2024-06-15T10:15:00Z",
  "duration_s":   900,
  "end_reason":   "card_removed",
  "status":       "completed"
}
```

`end_reason` values: `card_removed` | `timeout` | `remote_stop` | `power_loss` | `watchdog`

#### `/revoked/{card_uid}` (document ID = card UID hex string)
```json
{
  "uid":        "A1B2C3D4",
  "roll_number": "220002123",
  "revoked_at": "2024-06-15T12:00:00Z",
  "revoked_by": "admin_uid",
  "reason":     "lost"
}
```

#### `/admins/{uid}` and `/super_admins/{uid}`
```json
{
  "email":      "admin@iiti.ac.in",
  "name":       "Admin Name",
  "created_at": "2024-06-01T00:00:00Z",
  "is_active":  true
}
```

### RTDB Structure

```json
{
  "mms": {
    "nodes": {
      "1": {
        "state":            "active",
        "roll_number":      "220002123",
        "session_start":    1718600000,
        "firmware_version": "2.0.0",
        "ip_address":       "192.168.0.101",
        "last_seen":        1718600060,
        "rssi":             -55
      }
    },
    "commands": {
      "3": {
        "command":      "stop",
        "issued_by":    "admin_uid",
        "timestamp":    1718600100,
        "acknowledged": false
      }
    },
    "ota": {
      "firmware_url":   "https://firebasestorage.googleapis.com/...",
      "target_version": "2.1.0",
      "released_at":    1718600200
    },
    "revoked": {
      "uids":       ["A1B2C3D4", "E5F60718"],
      "updated_at": 1718600300
    },
    "bridge": {
      "status": {
        "state": "online",
        "ts":    1718600000
      }
    }
  }
}
```

---

## Verification Checklist

After completing all steps, verify:

```bash
# 1. Firestore rules deployed
firebase firestore:rules get
# Should print content of firestore.rules

# 2. RTDB rules deployed
firebase database:get / --output rules
# Or check Firebase Console → RTDB → Rules

# 3. Machines collection populated
firebase firestore:get /machines/1
# Should show the Creality 3D Printer document

# 4. Indexes built (check in console)
# Firestore → Indexes → all 3 should show "Enabled"

# 5. Storage accessible
firebase storage:get /
# Should show empty (no files yet) with no errors
```

---

## Common Mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| Wrong region for Firestore | High latency from India | Delete and recreate in `asia-south1` (Mumbai) |
| Indexes not deployed | Bridge throws `FAILED_PRECONDITION` on session deduplication | Run `firebase deploy --only firestore:indexes` |
| `machines` collection empty | Nodes never receive config via MQTT | Run `python3 scripts/seed_machines.py` |
| RTDB URL wrong in `.env` | Bridge can't write to RTDB | Copy URL exactly from Firebase Console → RTDB |
| Storage rules too permissive | Anyone can delete OTA binaries | Deploy restrictive storage rules |
| `sessions` write rule = true | Client can forge session records | Keep `allow write: if false` in `firestore.rules` |
