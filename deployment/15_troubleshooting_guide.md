# Section 15 — Troubleshooting Guide

← [Maintenance Handbook](14_maintenance_handbook.md) | → [Disaster Recovery](16_disaster_recovery.md)

---

## How to Use This Guide

Find the symptom that matches your problem, follow the tree from top to bottom. Each tree ends in either a fix or an escalation to disaster recovery (Section 16).

---

## Symptom Index

1. [Node OLED is blank or shows garbage](#1-node-oled-blank-or-garbage)
2. [Card inserted but no reaction](#2-card-inserted-but-no-reaction)
3. [Card denied with "Access Denied" — should be allowed](#3-card-denied-but-should-be-allowed)
4. [Relay does not energise after GRANTED](#4-relay-does-not-energise-after-granted)
5. [Machine does not power on despite relay energising](#5-machine-does-not-power-on-despite-relay-on)
6. [MQTT connection fails or keeps reconnecting](#6-mqtt-connection-fails)
7. [Sessions not appearing in Firestore](#7-sessions-not-in-firestore)
8. [Dashboard shows machine as "offline"](#8-dashboard-shows-offline)
9. [Remote stop not working](#9-remote-stop-not-working)
10. [Revocation not propagating to node](#10-revocation-not-propagating)
11. [OTA update stuck or failed](#11-ota-update-stuck-or-failed)
12. [Bridge service not starting](#12-bridge-service-not-starting)
13. [Node keeps rebooting (watchdog loop)](#13-node-keeps-rebooting)
14. [WiFi connection fails](#14-wifi-fails)
15. [WS2812 LED wrong colour or not working](#15-ws2812-wrong-colour)

---

## 1. Node OLED Blank or Garbage

```
OLED is completely blank
│
├── Is the node powered? (power LED on ESP32 lit?)
│   No → Check HiLink PSU output: measure Vin pin with multimeter
│         Should be 4.8–5.2V
│         No voltage → check AC fuse, HiLink wiring
│
└── Yes, node is powered
    │
    ├── Is it SPI OLED (128×64) or I2C OLED (128×32)?
    │
    ├── SPI OLED (128×64):
    │   Flash `04_oled_spi` test sketch
    │   Serial shows "SSD1306 init OK" but display blank?
    │   → Check CS (GPIO 16), DC (GPIO 17), RST (GPIO 4) wiring
    │   → Check 3.3V on VCC pin of display
    │   Serial shows "SSD1306 init FAILED"?
    │   → Check MOSI (GPIO 23) and SCK (GPIO 18) — SPI bus wiring
    │   → The SPI bus is shared with MFRC522; if RFID is broken it can corrupt SPI
    │
    └── I2C OLED (128×32):
        Flash `05_oled_i2c` test sketch
        Serial shows "SSD1306 I2C init OK" but blank?
        → Check SDA (GPIO 21), SCL (GPIO 22) wiring
        → Check I2C address: `i2cdetect` scan at address 0x3C
        Serial shows "SSD1306 I2C init FAILED"?
        → Confirm OLED module type: 128×32 SSD1306, I2C variant
        → Check 3.3V on VCC pin

OLED shows garbage or random pixels:
→ RST pin (SPI OLED only, GPIO 4) not held LOW long enough on init
→ Re-flash firmware; if persists, replace SSD1306 module
```

---

## 2. Card Inserted But No Reaction

```
Card inserted into HC89 slot — nothing happens on OLED, no serial output

├── Check serial: does any message appear at all?
│   No →
│       Is the HC89 sensor connected?
│       Flash `06_hc89` test sketch
│       Serial "HC89: ABSENT" → sensor wired, card not detected
│       → Check HC89 OUT → GPIO 32 connection
│       → Try a different card (HC89 slot width: card must fit snugly)
│       → Check 3.3V on HC89 VCC
│
│   Serial shows "[HC89] Card present" but no RFID activity →
│       MFRC522 is failing silently
│       Flash `02_rfid` test sketch
│       → Check SPI wiring: SS=GPIO5, SCK=GPIO18, MOSI=GPIO23, MISO=GPIO19, RST=GPIO27
│       → Check 3.3V on MFRC522 VCC (NOT 5V — MFRC522 is 3.3V only)
│
└── Serial shows RFID error:
    "[RFID] Auth FAILED" →
        Wrong card type (not MIFARE Classic 1K)
        OR card was written with a different master key
        Check card type — it must be MIFARE Classic 1K (Mifare S50)
        Verify master key in secrets.h matches kiosk secrets.py
```

---

## 3. Card Denied But Should Be Allowed

```
Serial shows "[ACCESS] DENIED" for a card that should be permitted

├── Reason: "card revoked"
│   → Admin accidentally revoked this card
│   → Web app → User Management → check if UID appears in revoked list
│   → Remove from revoked if revocation was a mistake (delete /revoked/{uid} in Firestore)
│   → Bridge will push updated revocation list; node accepts card within 5s
│
├── Reason: "permission bit = 0"
│   → Card was issued with wrong permissions
│   → Web app → User Management → check machine_permissions for this user
│   → If wrong: edit permissions → re-issue card at kiosk
│   → Old card cannot be updated remotely — must re-issue
│
├── Reason: "schema version != 0x02"
│   → Card was written by V1 kiosk (old system)
│   → Must re-issue card at kiosk with V2 schema
│
├── Reason: "machine disabled"
│   → Admin set machine_active = false
│   → Web app → Machine Management → toggle machine to enabled
│
└── Reason: "RFID auth failed"
    → Card was written by a different kiosk with different master key
    → OR sector key derivation mismatch between kiosk and node
    → Verify master key is identical (same 32 bytes) in:
        v2/access_node/secrets.h
        v2/kiosk_station/secrets.py (MASTER_KEY hex)
        v2/kiosk_station/station_writer/secrets_station.h
    → If mismatch confirmed: node must be re-flashed with correct key
      OR re-issue all cards with the node's key
```

---

## 4. Relay Does Not Energise After GRANTED

```
Serial shows "[RELAY] ON" but nothing happens

├── Check relay LED indicator (most relay modules have an LED):
│   LED on → relay is energising; problem is in the load circuit → see Symptom 5
│   LED off → relay control signal issue
│       Check GPIO 26 → Relay IN wiring
│       Measure GPIO 26 with multimeter during session: should be 3.3V (active HIGH)
│       If 0V: firmware not driving pin high → check relay_active_high NVS value
│         Serial: "[RELAY] relay_active_high = 0" → relay is active LOW
│         But if your relay is active HIGH, the config is wrong
│         Web app → Machine Management → relay type → set correctly
│
└── relay_active_high config:
    true  = GPIO HIGH to activate (most 5V mechanical relay modules)
    false = GPIO LOW to activate (some SSRs and optocoupler relay boards)
    Check your relay module datasheet
```

---

## 5. Machine Does Not Power On Despite Relay On

```
Relay is energising (LED on, click sound) but machine doesn't power on

├── Check load wiring:
│   COM and NO connected? (not NC)
│   Machine AC live wire on COM, machine input on NO?
│   Use multimeter: measure continuity from COM to NO when relay energised
│   Should show < 1 Ohm when relay is ON
│   Should show open circuit when relay is OFF
│
├── Is the relay rated for the load?
│   Check machine's current draw vs relay's rated current
│   Relay modules: typically 10A 250V AC
│   Laser cutter may exceed this → use SSR rated 25A+
│
└── Is the HiLink fuse blown?
    Measure HiLink output: 0V → check fuse
    Replace with same rating (5A)
```

---

## 6. MQTT Connection Fails

```
Serial: "[MQTT] Connection failed (rc=-2)" or repeated reconnect attempts

├── Can the node reach the RPi?
│   Check node's WiFi status: is WiFi connected? (OLED32 WiFi dot filled?)
│   If WiFi not connected → see Symptom 14
│
├── Is Mosquitto running on RPi?
│   ssh pi@192.168.0.10
│   sudo systemctl status mosquitto
│   Not running → sudo systemctl start mosquitto
│
├── Can a laptop reach Mosquitto?
│   From laptop on same WiFi:
│   mosquitto_sub -h 192.168.0.10 -p 1883 -t test &
│   mosquitto_pub -h 192.168.0.10 -p 1883 -t test -m hi
│   Not working → firewall blocking port 1883
│     sudo ufw allow 1883/tcp
│     sudo ufw reload
│
├── Is MQTT_BROKER IP in secrets.h correct?
│   Must match RPi's static IP (192.168.0.10 by default)
│   If RPi IP changed → update secrets.h, re-flash nodes
│
└── MQTT authentication enabled but credentials wrong?
    If mosquitto is configured with allow_anonymous false:
    Check MQTT_USER and MQTT_PASSWORD in secrets.h
    Confirm they match /etc/mosquitto/passwd on RPi
```

---

## 7. Sessions Not in Firestore

```
Sessions happening on node (relay on/off, serial confirms) but no Firestore documents

├── Is the bridge running?
│   curl http://192.168.0.10:8080/health
│   {"mqtt_connected": false, ...} → bridge lost MQTT → restart bridge
│   {"fb_connected": false, ...}   → Firebase connection lost → check internet on RPi
│   Connection refused              → bridge not running → sudo systemctl start mms-bridge
│
├── Bridge running but no Firestore writes?
│   sudo journalctl -u mms-bridge -f
│   Look for: "Firestore write failed" or "PERMISSION_DENIED"
│   PERMISSION_DENIED → Firestore rules blocking write
│     Check firestore.rules: sessions allow write: if false  ← bridge uses Admin SDK, bypasses rules
│     If bridge throws PERMISSION_DENIED, the service account may lack permissions
│     Firebase Console → IAM → ensure service account has "Firebase Admin" role
│
├── Is the event being published by the node?
│   Serial: "[MQTT] Published event: {...}" → event is sent
│   If no publish line → session manager not calling publish; check firmware
│
└── Bridge gets event but doesn't write?
    Check bridge log for the specific session event line
    If bridge log shows "Duplicate session detected — skipping":
    Two events were published for the same session (deduplication triggered)
    This is correct behaviour — no action needed
```

---

## 8. Dashboard Shows Machine as "Offline"

```
Web dashboard shows machine state as "offline"

├── Is the node actually powered and running?
│   Physical check: OLEDs lit? LED breathing?
│   If no → power issue → check HiLink, fuse
│
├── Is the node connected to MQTT?
│   Serial: "[MQTT] Connected"? Or reconnecting?
│   Node not connected → MQTT troubleshooting above (#6)
│
├── Is the bridge updating RTDB?
│   Check RTDB in Firebase Console → /mms/nodes/{id}/last_seen
│   Value not updating → bridge RTDB writes failing
│   sudo journalctl -u mms-bridge -f
│   Look for RTDB write errors
│
└── Is the web app reading RTDB correctly?
    Open browser dev tools → Network tab
    Look for RTDB websocket connection (wss://your-rtdb-url)
    If no websocket: VITE_FIREBASE_DATABASE_URL missing in .env → rebuild and redeploy web app
```

---

## 9. Remote Stop Not Working

```
Clicked Stop in dashboard — machine keeps running

├── Check RTDB: was the command written?
│   Firebase Console → /mms/commands/{machine_id}
│   Should show: {command:"stop", acknowledged:false, timestamp:...}
│   Not written → web app bug → check browser console for errors
│
├── Command written — did bridge forward it?
│   sudo journalctl -u mms-bridge -f | grep command
│   "[BRIDGE] Command forwarded to MQTT: {stop}" → bridge forwarded OK
│   No forward log → bridge not listening to /mms/commands RTDB path
│   Restart bridge: sudo systemctl restart mms-bridge
│
├── Bridge forwarded — did node receive it?
│   Serial: "[MQTT] Received command: stop"?
│   No → MQTT message not delivered → check QoS 1 pub/sub
│   Node may have been offline when command was published
│   Command has 5-min TTL — if >5min elapsed, it was discarded
│
└── Node received — but session didn't stop?
    Serial: "[CMD] State != SESSION_ACTIVE — command ignored"
    → The node was not in an active session when command arrived
    → This is correct behaviour; no action needed
```

---

## 10. Revocation Not Propagating to Node

```
Revoked card in web app but node still allows it

├── Did Firestore update?
│   Firebase Console → /revoked/{uid} → document should exist
│   Not created → web app revocation bug → check browser console
│
├── Did bridge push update to MQTT?
│   sudo journalctl -u mms-bridge -f | grep revoked
│   "[BRIDGE] Revocation list pushed: 1 UID(s)" → push happened
│   Not in logs → Firestore onSnapshot listener not triggering
│   Restart bridge: sudo systemctl restart mms-bridge
│
├── Did node receive update?
│   Serial: "[MQTT] Received revoked list — 1 UID(s)"?
│   Not received → node offline at time of push
│   Retained message: node will receive it on next MQTT connect
│   Wait for node to reconnect, then test card again
│
└── Node received update but card still allowed?
    Serial: "[ACCESS] Revocation check — UID found — DENIED"?
    If not denied: check that UID in DENIED list matches card UID exactly
    UID format: uppercase hex without spaces, e.g., "A1B2C3D4"
    Compare: Serial RFID UID vs Firebase revoked document ID
```

---

## 11. OTA Update Stuck or Failed

```
Node shows "Updating FW..." on OLED but doesn't complete

├── Check serial output:
│   "[OTA] Downloading from URL: ..."
│   "[OTA] Error: -1" or timeout
│   → URL may be expired (Firebase Storage signed URLs expire after 1 hour)
│   → Generate a new download URL; update RTDB /mms/ota/firmware_url
│
├── OTA completes but node keeps restarting:
│   → New firmware has crash bug
│   → Rollback activates after 3 crashes (automatic)
│   → Node will revert to previous working firmware
│   → Fix the firmware bug, rebuild, re-upload to Storage, re-trigger OTA
│
├── OTA never starts:
│   → Bridge may not have forwarded OTA command
│   → Check RTDB /mms/ota was written correctly
│   → Check bridge log for "[BRIDGE] OTA broadcast"
│   → If no broadcast: RTDB OTA listener may not be registered
│   → Restart bridge
│
└── Node in error state after OTA, rollback failed:
    Manual recovery required:
    1. Connect laptop via USB
    2. Flash known-good firmware via Arduino IDE
    3. Reprovisioned and re-tested before reinstalling
    → See Section 16 for firmware recovery procedure
```

---

## 12. Bridge Service Not Starting

```
sudo systemctl status mms-bridge → "failed" or "activating (start)"

├── Check error log:
│   sudo journalctl -u mms-bridge -n 50
│   
│   "ModuleNotFoundError: No module named 'firebase_admin'"
│   → Python packages not installed in venv
│   → source /home/pi/mms/venv/bin/activate && pip install firebase-admin paho-mqtt
│
│   "FileNotFoundError: service-account.json"
│   → Service account file not at /home/pi/mms/service-account.json
│   → Re-transfer from secure storage
│
│   "CredentialsMissingError" or "invalid_grant"
│   → Service account key is revoked or expired
│   → Generate new key from Firebase Console → Project Settings → Service Accounts
│   → Transfer and update file, restart bridge
│
│   "MQTT connect refused"
│   → Mosquitto not running
│   → sudo systemctl start mosquitto
│   → Then restart bridge: sudo systemctl restart mms-bridge
│
└── Bridge starts but crashes within minutes:
    → Check for uncaught exception in log
    → File a bug report with the full traceback
    → In the meantime, manually run bridge.py to observe output:
      source /home/pi/mms/venv/bin/activate
      python3 /home/pi/mms/bridge.py
```

---

## 13. Node Keeps Rebooting

```
Node boots, prints some output, then reboots repeatedly

├── Check serial for boot reason:
│   "[SYS] Boot reason: WDT reset" → watchdog triggered
│       → Something is blocking the main loop for >30s
│       → Connect serial monitor and watch where it stalls
│       → Common cause: blocking MQTT reconnect, I2C lockup
│
│   "[SYS] Boot reason: panic/exception"
│       → Firmware bug; note the exception details (core dump)
│       → Report with full serial output
│
├── Bootloop after OTA:
│   → Automatic rollback should activate after 3 crashes
│   → If rollback works: node returns to previous version (check serial for "rolled back")
│   → If rollback fails or node is on first boot ever:
│       Flash via USB with Arduino IDE
│
└── Hardware causing reset (brown-out):
    Node resets when relay energises?
    → Power supply (HiLink) is under-rated or loose connection
    → Relay inrush current causing voltage dip → ESP32 brown-out reset
    → Add 100µF electrolytic capacitor across ESP32 Vin–GND
    → Check HiLink connections, consider HLK-PM03 (3W) instead of HLK-PM01 (1W)
```

---

## 14. WiFi Fails

```
Serial: "[NET] WiFi failed after 10s" or keeps trying

├── Check SSID and password in secrets.h
│   Common mistake: wrong case, trailing space, wrong SSID for 5GHz vs 2.4GHz
│   ESP32 only supports 2.4GHz WiFi — must use 2.4GHz band AP
│
├── Is the node too far from AP?
│   Bring node close to AP and test
│   If it works close but not at installation location:
│   → Add a WiFi range extender in the lab
│   → Typical ESP32 range: ~30m in open air, less with walls
│
├── Is the AP at capacity?
│   Home/budget routers often limit to 32 or 64 concurrent devices
│   If lab already has many devices: check AP's connected device count
│   → Upgrade AP or add second AP for MMS devices
│
└── Node connects but RSSI < -80 dBm (shown on OLED32 or serial):
    → Signal is too weak; expect intermittent disconnects
    → Relocate node or add AP
    → Acceptable RSSI: -30 to -67 dBm (good), -68 to -79 dBm (fair)
```

---

## 15. WS2812 Wrong Colour

```
LED is wrong colour (e.g., shows blue when it should be green)

├── This is a colour order issue
│   Default in firmware: NEO_GRB
│   Some WS2812 clones use RGB or BGR order
│   If red shows as green and vice versa: change to NEO_RGB
│   If red shows as blue: change to NEO_BRG
│   Update in led_controller.cpp and rebuild
│
├── LED not responding at all:
│   Check 470Ω resistor on DIN line (GPIO 33 → [470Ω] → DIN)
│   Check 5V on VCC (not 3.3V)
│   Check 100µF capacitor polarity (+ to VCC)
│   Flash `07_ws2812` test sketch to isolate
│
└── LED flickering or going white randomly:
    → Power supply noise
    → Add 100µF capacitor AT the LED (as close as possible, not at ESP32)
    → Check GND is connected
```

---

## Escalation Path

If a troubleshooting tree does not resolve the issue:

1. Document the full serial output (copy-paste from Serial Monitor)
2. Document the bridge log at the time of failure:
   `sudo journalctl -u mms-bridge --since "30 min ago"`
3. Document the RTDB and Firestore state at the time
4. Proceed to [Section 16 — Disaster Recovery](16_disaster_recovery.md) for component replacement procedures
