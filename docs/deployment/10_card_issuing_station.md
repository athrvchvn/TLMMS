# Section 10 — Card Issuing Station

← [Web Application Deployment](09_web_application_deployment.md) | → [Access Node Provisioning](11_access_node_provisioning.md)

---

## Why This Section Exists

The card issuing station (kiosk) is where physical RFID cards are personalised for each student. It encodes the student's roll number, PIN hash, and machine permission bitmask onto a MIFARE Classic 1K card using a cryptographic sector key derived from the master key. Without this station, no student can use any machine.

---

## How Card Issuance Works

```
Admin opens kiosk web UI
  → Selects student from database (or enters new student info)
  → Selects which machines student is allowed to use
  → Clicks "Issue Card"

Flask app (kiosk_station/app.py):
  1. Computes permission bitmask from selected machines
  2. Computes PIN hash: HMAC-SHA256(master_key ∥ uid_hex, pin_string)[0:7]
     (8 bytes = 16 hex chars)
  3. Sends JSON command to station_writer.ino via serial:
     {"roll":"220002123","perm_mask":15,"pin_hash":"aabbccddaabbccdd","uid":"A1B2C3D4"}

station_writer.ino (on ESP32/Arduino):
  1. Waits for card on MFRC522
  2. Reads UID
  3. Derives sector key: HMAC-SHA256(master_key, uid_hex_uppercase)[0:5]
  4. Authenticates Sector 1 with default key (fresh card) or derived key (re-issue)
  5. Writes Block 4: roll_number[0:8] + 0x02 (schema byte)
  6. Writes Block 5: pin_hash[0:7] + perm_mask (uint32 LE) 
  7. Updates Sector 1 trailer: Key A = derived sector key
  8. Verifies Block 4 schema byte = 0x02
  9. Reports "WRITE_SUCCESS" or "WRITE_FAILED"

Flask app receives result → shows success/failure to admin
```

---

## Hardware Assembly

The kiosk needs:
1. **Computer** (laptop, RPi, or any Linux/Windows/Mac with Python 3.10+)
2. **MFRC522 module** connected to an Arduino/ESP32 via USB serial
3. **USB cable** from computer to Arduino/ESP32

**Option A — ESP32 as station writer (recommended):**
```
Laptop USB ←──→ ESP32 (station_writer.ino flashed)
                  └── MFRC522 (SPI, same wiring as node)
```

**Option B — Arduino Nano/Uno as station writer:**
```
Laptop USB ←──→ Arduino Nano
                  └── MFRC522 (SPI)
                       SS → D10, SCK → D13, MOSI → D11, MISO → D12, RST → D9
                       (Arduino SPI pins are different from ESP32)
```

Note: If using Arduino Nano, the wiring is different from the ESP32 node. Only the ESP32 version uses the firmware in `station_writer/station_writer.ino`.

---

## Step 1 — Generate the Master Key (ONE TIME ONLY)

**This must be done before flashing any node or issuing any card. Do it exactly once.**

```bash
python3 -c "
import secrets
bs = secrets.token_bytes(32)
print('C array (for secrets.h and secrets_station.h):')
print('  ' + ', '.join(f'0x{b:02X}' for b in bs))
print()
print('Python hex (for secrets.py):')
print('  ' + bs.hex())
"
```

Output example:
```
C array (for secrets.h and secrets_station.h):
  0xA1, 0x3F, 0x88, ..., 0x5C

Python hex (for secrets.py):
  a13f88...5c
```

**Store the output securely (password manager, printed in sealed envelope, etc.). This key is irreplaceable — losing it means all existing cards must be replaced.**

---

## Step 2 — Flash the Station Writer Firmware

```bash
# In Arduino IDE:
# 1. Open apps/kiosk_station/station_writer/station_writer.ino
# 2. Copy station_writer/secrets_station.h.template to station_writer/secrets_station.h
# 3. Fill in the master key bytes in secrets_station.h
# 4. Select board: ESP32 Dev Module (or Arduino Nano if using that)
# 5. Upload
```

In `secrets_station.h`:
```cpp
static const uint8_t MASTER_KEY[32] = {
    0xA1, 0x3F, 0x88, 0x...,  // fill in from generation step
    ...
};
```

**Verify firmware is correct:** Open Serial Monitor at 115200 baud. Should print:
```
MMS_WRITER_READY
```

If it prints:
```
FATAL: MASTER_KEY is all zeros in secrets_station.h
```
The template was not filled in. Fill in the actual key bytes and re-flash.

---

## Step 3 — Set Up the Python Kiosk App

```bash
cd apps/kiosk_station

# Copy secrets template
cp secrets.py.template secrets.py
nano secrets.py
```

In `secrets.py`:
```python
MASTER_KEY = bytes.fromhex("a13f88...5c")  # 64 hex chars = 32 bytes
```

```bash
# Install dependencies
pip install flask firebase-admin pyserial

# Or use requirements.txt:
pip install -r requirements.txt

# Set Firebase credentials
export GOOGLE_APPLICATION_CREDENTIALS=/path/to/service-account.json
```

---

## Step 4 — Configure the Port

```bash
# Find the serial port the station writer is on
# Linux:
ls /dev/tty*
# Look for /dev/ttyUSB0 or /dev/ttyACM0

# Mac:
ls /dev/cu.*
# Look for /dev/cu.SLAB_USBtoUART or /dev/cu.usbserial-...
```

In `apps/apps/kiosk_station/config.py`:
```python
RFID_PORT = "/dev/ttyUSB0"  # change to match your port
```

---

## Step 5 — Start the Kiosk App

```bash
cd apps/kiosk_station
python3 app.py

# Expected output:
# * Running on http://127.0.0.1:5000
# * Running on http://192.168.X.X:5000
```

Open a browser and go to `http://localhost:5000`.

---

## Card Issuance Workflow

### Issuing a new card

1. Admin creates the student in the web app first (see Section 09 Step 5)
   - This creates `/users/{roll_number}` in Firestore
2. Open kiosk at `http://localhost:5000`
3. Select the student's roll number from the dropdown
4. Confirm machine permissions shown match what was set in web app
5. Click "Issue Card"
6. The app says "AWAITING_CARD — place card on reader"
7. Place a **blank** MIFARE Classic 1K card on the MFRC522
8. Wait for "WRITE_SUCCESS"
9. The kiosk updates Firestore: `/users/{roll_number}` gets `card_uid`, `issued_at`

**Verify:** Take the card to any machine node. Insert it. The OLED should show the student's roll number and grant access (for permitted machines).

### Re-issuing a card (lost/replacement)

1. First, revoke the old card:
   - Web app → User Management → select student → Revoke Card
   - This creates `/revoked/{old_uid}` in Firestore
   - Bridge pushes updated revocation list to all nodes
2. At kiosk: issue a new card (same procedure as new card)
   - The new UID generates a new (different) sector key
   - Old card will be denied even if someone finds it

### Card replacement (damaged, not lost)

If the card was damaged but not stolen:
1. Read the old card UID: at kiosk, place old card → "Read UID" button
2. No need to revoke (the physical card is destroyed)
3. Issue a new card normally

---

## PIN Reset Workflow

If a student forgets their PIN:
1. Admin in web app → User Management → select student → "Reset PIN"
2. Admin enters a new 4-digit PIN
3. Web app computes new PIN hash via `hashPin()` in `src/lib/firebase.ts`
4. Updates `/users/{roll_number}.pin_hash`
5. The student must get a **new card issued** — the PIN hash is stored on the physical card
   - Existing card still has old PIN hash; it can be used for machine access (PIN not verified at machine in V2)
   - New card will have the new PIN hash (needed for kiosk login only)

---

## Failure Recovery Procedures

### "WRITE_FAILED" response
```
Causes:
1. Card was moved during write (authentication loss between blocks)
2. Card is not MIFARE Classic 1K (NFC tags, bank cards, transit cards don't work)
3. Card was previously written with different master key (sector auth fails)
4. Serial disconnect between Flask app and station writer during operation

Recovery:
1. Try the same card again — if card was moved, retry often works
2. Use a different card from the stack
3. If card is from another system, discard it
4. Restart station_writer Arduino, restart Flask app, retry
```

### "UID_MISMATCH" response
```
Cause: Flask app computed bitmask for a specific UID, but a different card was placed.
Recovery: Remove the card, click "Issue Card" again, place the correct card.
```

### Station writer not responding (no serial output)
```
1. Unplug and replug USB cable
2. Check Serial Monitor: should show "MMS_WRITER_READY" on connect
3. If not: re-flash station_writer.ino
4. Check RFID_PORT in config.py matches actual port
```

### Flask app can't connect to Firebase
```
1. Check GOOGLE_APPLICATION_CREDENTIALS is set and file exists
2. Check internet connectivity on kiosk machine
3. Check service account has Firestore read/write permissions
```
