# Section 07 — Raspberry Pi Deployment

← [Hardware Validation](06_hardware_validation.md) | → [Firebase Deployment](08_firebase_deployment.md)

---

## Why This Section Exists

The Raspberry Pi is the communication hub of MMS V2. It runs Mosquitto (the MQTT broker) and `bridge.py` (the Firebase bridge). If the RPi is down, nodes continue to operate offline, but no session data reaches Firebase and no admin commands reach the nodes. This section documents how to set up the RPi from a fresh OS install.

---

## Hardware Requirements

| Item | Specification |
|---|---|
| Model | Raspberry Pi 4 Model B (2GB) or RPi 3B+ |
| Power | Official RPi 5.1V 3A USB-C supply (NOT a phone charger) |
| Storage | 32GB Class 10 microSD (Samsung Endurance Pro recommended) |
| Network | **Ethernet** to router — DO NOT use RPi WiFi as the broker |
| Static IP | Set via router DHCP reservation OR `/etc/dhcpcd.conf` |

**Why Ethernet?** WiFi on the RPi can have intermittent latency spikes that cause MQTT timeouts. All nodes connect to the broker via WiFi — if the broker itself is also on WiFi, you have two wireless hops. Ethernet to the router is free of this problem.

---

## Step 1 — Operating System

**OS:** Raspberry Pi OS Lite (64-bit), Debian Bookworm (2024-03-15 or later). No desktop needed.

**Flash the SD card:**
```bash
# On your laptop
# Download Raspberry Pi Imager from https://www.raspberrypi.com/software/
# Select: Raspberry Pi OS Lite (64-bit)
# Settings (gear icon):
#   Enable SSH: yes
#   Username: pi
#   Password: <strong password>
#   WiFi: configure if needed for initial setup (switch to Ethernet after)
#   Hostname: mms-bridge
```

**First boot:**
```bash
# From your laptop, SSH in:
ssh pi@mms-bridge.local
# or by IP:
ssh pi@<dhcp-assigned-ip>

# Update packages
sudo apt update && sudo apt upgrade -y

# Set timezone
sudo timedatectl set-timezone Asia/Kolkata

# Verify time
date
# Should show IST (UTC+5:30)
```

---

## Step 2 — Static IP Configuration

The RPi MUST have a static IP. All ESP32 nodes use `MQTT_BROKER` in `config.h` to connect — if the RPi's IP changes, all nodes fail to connect.

**Method A (preferred): DHCP reservation in router**
- Open your router admin page (usually 192.168.0.1)
- Find the RPi's MAC address: `ip link show eth0 | grep ether`
- Reserve IP `192.168.0.10` for that MAC address
- Restart RPi

**Method B: Static IP in `/etc/dhcpcd.conf`**
```bash
sudo nano /etc/dhcpcd.conf
```
Add at the bottom:
```
interface eth0
static ip_address=192.168.0.10/24
static routers=192.168.0.1
static domain_name_servers=8.8.8.8 8.8.4.4
```
```bash
sudo systemctl restart dhcpcd
ip addr show eth0
# Should now show 192.168.0.10
```

**Verify from an ESP32 node:** After nodes are flashed, check serial output for `[MQTT] Connected to 192.168.0.10:1883`.

---

## Step 3 — Install System Packages

```bash
sudo apt install -y \
    mosquitto \
    mosquitto-clients \
    python3 \
    python3-pip \
    python3-venv \
    git \
    htop \
    vim \
    curl
```

**Verify:**
```bash
mosquitto -v &
# Should print: mosquitto version X.X.X starting
# Then: Opening ipv4 listen socket on port 1883
kill %1
```

---

## Step 4 — Mosquitto Configuration

By default Mosquitto only accepts connections from localhost. Configure it to accept LAN connections.

```bash
sudo nano /etc/mosquitto/conf.d/mms.conf
```

```ini
# MMS V2 Mosquitto configuration

# Accept connections from any LAN address
listener 1883 0.0.0.0

# For initial deployment: allow anonymous (secure later with passwords)
allow_anonymous true

# Enable message persistence (required for retained message delivery after broker restart)
persistence true
persistence_location /var/lib/mosquitto/

# Logging
log_dest syslog
log_type all
log_timestamp true

# Connection limits
max_connections 50
max_queued_messages 1000
```

```bash
sudo systemctl enable mosquitto
sudo systemctl restart mosquitto
sudo systemctl status mosquitto
# Should show: Active: active (running)
```

**Test from your laptop:**
```bash
# Install mosquitto clients on laptop
# Ubuntu: sudo apt install mosquitto-clients
# Mac: brew install mosquitto

# Subscribe to a test topic
mosquitto_sub -h 192.168.0.10 -p 1883 -t "mms/test" &

# Publish a test message
mosquitto_pub -h 192.168.0.10 -p 1883 -t "mms/test" -m "hello"

# Should see "hello" in the subscriber output
```

**If the test fails:** Check firewall — see Step 8.

### Adding MQTT Authentication (After Initial Testing)

Once all nodes are working, enable authentication:
```bash
# Create password file
sudo mosquitto_passwd -c /etc/mosquitto/passwd bridge

# Add each node
sudo mosquitto_passwd /etc/mosquitto/passwd node_1
sudo mosquitto_passwd /etc/mosquitto/passwd node_2
# ... repeat for each node

# Update /etc/mosquitto/conf.d/mms.conf:
# Change: allow_anonymous true
# To:     allow_anonymous false
#         password_file /etc/mosquitto/passwd

sudo systemctl restart mosquitto
```

Update each node's `secrets.h` with the correct `MQTT_USER`/`MQTT_PASSWORD` and re-flash.

---

## Step 5 — Python Environment Setup

```bash
# Create project directory
mkdir -p /home/pi/mms
cd /home/pi/mms

# Copy bridge.py from repository
# Option A: clone the repo
git clone <your-repo-url> repo
cp repo/services/rpi_bridge/bridge.py /home/pi/mms/
cp repo/services/rpi_bridge/.env.template /home/pi/mms/.env

# Option B: SFTP upload from laptop
# sftp pi@192.168.0.10 → put services/rpi_bridge/bridge.py /home/pi/mms/

# Create virtual environment
python3 -m venv /home/pi/mms/venv

# Activate and install dependencies
source /home/pi/mms/venv/bin/activate
pip install \
    firebase-admin==6.5.0 \
    paho-mqtt==1.6.1 \
    python-dotenv==1.0.0

# Optional: systemd watchdog integration
pip install systemd-python

deactivate
```

---

## Step 6 — Firebase Admin SDK Setup

The bridge uses the Firebase Admin SDK to write to Firestore and RTDB. It needs a service account JSON file.

**Generate service account:**
1. Go to [Firebase Console](https://console.firebase.google.com) → your project
2. Click the gear icon → Project Settings → Service Accounts
3. Click "Generate new private key"
4. Download `serviceAccountKey.json`
5. Transfer to RPi:
```bash
sftp pi@192.168.0.10
put serviceAccountKey.json /home/pi/mms/service-account.json
exit
```

**Verify permissions:**
```bash
chmod 600 /home/pi/mms/service-account.json
ls -la /home/pi/mms/service-account.json
# Should show: -rw------- 1 pi pi ...
```

---

## Step 7 — Environment File

```bash
nano /home/pi/mms/.env
```

```env
# MMS Bridge environment configuration
MQTT_BROKER=localhost
MQTT_PORT=1883
MQTT_USER=bridge
MQTT_PASS=your_bridge_mqtt_password

GOOGLE_APPLICATION_CREDENTIALS=/home/pi/mms/service-account.json
FIREBASE_RTDB_URL=https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app
```

**How to find your RTDB URL:**
- Firebase Console → Realtime Database → the URL shown at the top (e.g., `https://lab-access-f569b-default-rtdb.asia-southeast1.firebasedatabase.app`)

---

## Step 8 — Firewall Configuration

```bash
# Check current firewall status
sudo ufw status

# If enabled, open MQTT port
sudo ufw allow 1883/tcp comment "MQTT"
sudo ufw allow ssh
sudo ufw reload

# If firewall is inactive, iptables may still block — check:
sudo iptables -L INPUT | grep 1883
```

---

## Step 9 — Systemd Service

```bash
sudo cp /home/pi/mms/repo/services/rpi_bridge/mms-bridge.service /etc/systemd/system/

# Or create manually:
sudo nano /etc/systemd/system/mms-bridge.service
```

```ini
[Unit]
Description=MMS Firebase Bridge
After=network-online.target mosquitto.service
Wants=network-online.target

[Service]
Type=notify
User=pi
WorkingDirectory=/home/pi/mms
EnvironmentFile=/home/pi/mms/.env
ExecStart=/home/pi/mms/venv/bin/python3 /home/pi/mms/bridge.py
Restart=always
RestartSec=5
StartLimitIntervalSec=60
StartLimitBurst=5
WatchdogSec=60
NotifyAccess=main

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable mms-bridge
sudo systemctl start mms-bridge

# Check status
sudo systemctl status mms-bridge

# Should show:
# Active: active (running)
# Main PID: XXXX (python3)
```

---

## Step 10 — Validate Bridge Operation

```bash
# Watch bridge logs in real time
sudo journalctl -u mms-bridge -f

# Expected log output after start:
# [INFO] mms_bridge: Firebase initialized — Firestore + RTDB ready
# [INFO] mms_bridge: MQTT connected to localhost:1883
# [INFO] mms_bridge: RTDB command listener active
# [INFO] mms_bridge: RTDB OTA listener active
# [INFO] mms_bridge: Firestore machines config listener active
# [INFO] mms_bridge: Firestore revocation listener active
# [INFO] mms_bridge: systemd notified: READY
# [INFO] mms_bridge: MMS Bridge running — MQTT=localhost:1883 Firebase=...
```

**Health check endpoint:**
```bash
curl http://localhost:8080/health
# Expected: {"mqtt_connected": true, "fb_connected": true, "uptime_s": 42}
```

---

## Directory Structure

After setup, the RPi should look like this:
```
/home/pi/mms/
├── bridge.py                ← Main bridge service
├── service-account.json     ← Firebase credentials (chmod 600)
├── .env                     ← Environment variables
└── venv/                    ← Python virtual environment
    └── lib/python3.11/site-packages/
        ├── firebase_admin/
        ├── paho/
        └── ...

/etc/mosquitto/conf.d/
└── mms.conf                 ← Mosquitto configuration

/etc/systemd/system/
└── mms-bridge.service       ← systemd unit file

/var/lib/mosquitto/
└── mosquitto.db             ← Persisted retained messages
```

---

## Backup Strategy

**What to back up:**

| File | Location | How often | Method |
|---|---|---|---|
| `service-account.json` | `/home/pi/mms/` | Once (never changes) | SFTP to secure storage |
| `.env` | `/home/pi/mms/` | When changed | SFTP to secure storage |
| `bridge.py` | `/home/pi/mms/` | When updated | Git push to repo |
| Mosquitto DB | `/var/lib/mosquitto/` | Weekly | `rsync` to external |
| Full SD image | Entire card | Monthly | `dd` |

**Quick SD image backup:**
```bash
# From laptop (with SD in card reader):
# Linux:
sudo dd if=/dev/sdX of=mms-bridge-backup-$(date +%Y%m%d).img bs=4M status=progress

# Or use Raspberry Pi Imager → "Read" option
```

---

## Common Mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| Using phone charger for RPi | Random reboots under load | Use official RPi 5.1V 3A supply |
| RPi on WiFi (not Ethernet) | Intermittent MQTT disconnects | Connect Ethernet cable |
| Dynamic IP not reserved | Nodes can't connect after router restart | Set static IP or DHCP reservation |
| Wrong RTDB URL in `.env` | Bridge starts but Firestore events don't appear | Check Firebase Console RTDB URL exactly |
| `service-account.json` wrong permissions | `PERMISSION_DENIED` in bridge logs | `chmod 600 service-account.json` |
| Mosquitto `allow_anonymous false` before updating nodes | All nodes rejected | Add authentication to nodes first, then disable anonymous |
| Bridge not enabled in systemd | Bridge not running after reboot | `sudo systemctl enable mms-bridge` |
