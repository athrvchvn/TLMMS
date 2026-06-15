#!/usr/bin/env python3
"""
MMS V2 — Raspberry Pi Firebase Bridge
Connects Mosquitto MQTT broker to Firebase (Firestore + RTDB).

Responsibilities:
  1. MQTT events → Firestore session records
  2. MQTT status → RTDB node state
  3. RTDB commands → MQTT command topics
  4. Firestore machines onSnapshot → MQTT retained config topics
  5. Firestore revoked onSnapshot → MQTT retained revocation list
  6. Periodic stale command cleanup (10-minute sweep)
  7. Duplicate session deduplication (power-failure safety)
  8. systemd watchdog integration
"""

import json
import logging
import os
import signal
import sys
import threading
import time
from datetime import datetime, timezone
from typing import Optional

import paho.mqtt.client as mqtt
import firebase_admin
from firebase_admin import credentials, firestore, db as rtdb
from google.cloud.firestore_v1 import SERVER_TIMESTAMP

try:
    import systemd.daemon as sd_daemon
    HAVE_SYSTEMD = True
except ImportError:
    HAVE_SYSTEMD = False

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
MQTT_BROKER   = os.environ.get("MQTT_BROKER", "localhost")
MQTT_PORT     = int(os.environ.get("MQTT_PORT", 1883))
MQTT_USER     = os.environ.get("MQTT_USER", "bridge")
MQTT_PASS     = os.environ.get("MQTT_PASS", "changeme")
MQTT_KEEPALIVE = 60

SERVICE_ACCOUNT = os.environ.get("GOOGLE_APPLICATION_CREDENTIALS", "/home/pi/mms/service-account.json")
RTDB_URL        = os.environ.get("FIREBASE_RTDB_URL", "https://lab-access-f569b-default-rtdb.firebaseio.com")

STALE_COMMAND_TTL_S   = 300   # 5 minutes
COMMAND_CLEANUP_INTERVAL_S = 600  # 10-minute sweep
WATCHDOG_INTERVAL_S   = 25    # notify systemd every 25s (WatchdogSec=60)
HEALTH_PORT           = 8080

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
log = logging.getLogger("mms_bridge")

# ---------------------------------------------------------------------------
# State
# ---------------------------------------------------------------------------
_mqtt_client: Optional[mqtt.Client] = None
_fs_client = None      # Firestore
_rtdb_ref = None       # Firebase RTDB root ref
_mqtt_connected = False
_fb_connected   = False
_start_time     = time.time()
_listeners      = []   # Firestore onSnapshot listener unsubscribe handles


# ---------------------------------------------------------------------------
# Firebase initialisation
# ---------------------------------------------------------------------------
def init_firebase():
    global _fs_client, _rtdb_ref, _fb_connected
    cred = credentials.Certificate(SERVICE_ACCOUNT)
    firebase_admin.initialize_app(cred, {"databaseURL": RTDB_URL})
    _fs_client = firestore.client()
    _rtdb_ref  = rtdb.reference("/mms")
    _fb_connected = True
    log.info("Firebase initialized — Firestore + RTDB ready")


# ---------------------------------------------------------------------------
# MQTT helpers
# ---------------------------------------------------------------------------
def mqtt_publish(topic: str, payload: dict | str, qos: int = 1, retain: bool = False):
    if isinstance(payload, dict):
        payload = json.dumps(payload, separators=(",", ":"))
    if _mqtt_client and _mqtt_connected:
        _mqtt_client.publish(topic, payload, qos=qos, retain=retain)
    else:
        log.warning("MQTT not connected — dropped publish to %s", topic)


# ---------------------------------------------------------------------------
# MQTT → Firebase: event handler
# ---------------------------------------------------------------------------
def handle_node_event(machine_id: str, payload: dict):
    """Write a session event from an ESP32 node into Firestore."""
    try:
        event_type = payload.get("e", "")
        roll       = payload.get("r", "")
        ts_unix    = payload.get("t", int(time.time()))
        duration_s = payload.get("d")
        end_reason = payload.get("er", "")

        ts = datetime.fromtimestamp(ts_unix, tz=timezone.utc)

        if event_type == "session_end":
            # Deduplication: check for existing record within ±5s
            existing = (
                _fs_client.collection("sessions")
                .where("machine_id", "==", machine_id)
                .where("roll_number", "==", roll)
                .order_by("start_time", direction=firestore.Query.DESCENDING)
                .limit(1)
                .get()
            )
            if existing:
                doc = existing[0].to_dict()
                existing_start = doc.get("start_time")
                if existing_start:
                    existing_ts = existing_start.timestamp()
                    expected_start = ts_unix - (duration_s or 0)
                    if abs(existing_ts - expected_start) < 5:
                        log.info("Duplicate session skipped — machine %s roll %s ts %d", machine_id, roll, ts_unix)
                        return

            # Resolve machine name
            machine_doc = _fs_client.collection("machines").document(machine_id).get()
            machine_name = machine_doc.to_dict().get("name", f"Machine {machine_id}") if machine_doc.exists else f"Machine {machine_id}"

            # Resolve user name
            user_doc = _fs_client.collection("users").document(roll).get()
            user_name = user_doc.to_dict().get("name", "") if user_doc.exists else ""

            start_ts = datetime.fromtimestamp(ts_unix - (duration_s or 0), tz=timezone.utc)

            session_data = {
                "machine_id":   machine_id,
                "machine_name": machine_name,
                "roll_number":  roll,
                "user_name":    user_name,
                "start_time":   start_ts,
                "end_time":     ts,
                "duration_s":   duration_s,
                "end_reason":   end_reason,
                "status":       "completed",
            }
            _fs_client.collection("sessions").add(session_data)
            log.info("Session written — machine %s roll %s duration %ss", machine_id, roll, duration_s)

            # Update user last_seen
            if roll:
                _fs_client.collection("users").document(roll).update({"last_seen": ts})

        elif event_type == "boot":
            # Close any orphaned active session for this machine
            orphans = (
                _fs_client.collection("sessions")
                .where("machine_id", "==", machine_id)
                .where("status", "==", "active")
                .get()
            )
            for orphan in orphans:
                orphan.reference.update({
                    "status":     "completed",
                    "end_time":   ts,
                    "end_reason": "power_loss",
                    "duration_s": 0,
                })
                log.info("Orphaned session closed on boot — machine %s doc %s", machine_id, orphan.id)

    except Exception:
        log.exception("handle_node_event failed — machine %s", machine_id)


# ---------------------------------------------------------------------------
# MQTT → RTDB: status handler
# ---------------------------------------------------------------------------
def handle_node_status(machine_id: str, payload: dict):
    """Mirror ESP32 status to RTDB /mms/nodes/{id}."""
    try:
        _rtdb_ref.child(f"nodes/{machine_id}").update({
            "state":            payload.get("state", "unknown"),
            "roll_number":      payload.get("roll"),
            "session_start":    payload.get("session_start"),
            "firmware_version": payload.get("fw_version", ""),
            "ip_address":       payload.get("ip", ""),
            "last_seen":        payload.get("ts", int(time.time())),
            "rssi":             payload.get("rssi", 0),
        })
    except Exception:
        log.exception("handle_node_status failed — machine %s", machine_id)


# ---------------------------------------------------------------------------
# MQTT callbacks
# ---------------------------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    global _mqtt_connected
    if rc == 0:
        _mqtt_connected = True
        log.info("MQTT connected to %s:%d", MQTT_BROKER, MQTT_PORT)
        client.subscribe("mms/nodes/+/event", qos=1)
        client.subscribe("mms/nodes/+/status", qos=1)
        client.subscribe("mms/nodes/+/heartbeat", qos=0)
    else:
        log.error("MQTT connect failed — rc=%d", rc)


def on_disconnect(client, userdata, rc):
    global _mqtt_connected
    _mqtt_connected = False
    if rc != 0:
        log.warning("MQTT unexpected disconnect rc=%d — will reconnect", rc)


def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = json.loads(msg.payload.decode("utf-8"))
        parts = topic.split("/")

        if len(parts) != 4 or parts[0] != "mms" or parts[1] != "nodes":
            return

        machine_id = parts[2]
        msg_type   = parts[3]

        if msg_type == "event":
            handle_node_event(machine_id, payload)
        elif msg_type == "status":
            handle_node_status(machine_id, payload)
        # heartbeat: no action needed currently
    except json.JSONDecodeError:
        log.warning("Non-JSON MQTT message on %s", msg.topic)
    except Exception:
        log.exception("on_message unhandled exception — topic %s", msg.topic)


def on_publish(client, userdata, mid):
    pass


# ---------------------------------------------------------------------------
# RTDB listeners: commands → MQTT
# ---------------------------------------------------------------------------
def setup_rtdb_command_listener():
    """Watch /mms/commands/* and forward to MQTT command topics."""
    commands_ref = _rtdb_ref.child("commands")

    def on_commands_change(event):
        try:
            if event.data is None:
                return
            data = event.data
            if not isinstance(data, dict):
                return

            now = int(time.time())
            for machine_id, cmd_data in data.items():
                if not isinstance(cmd_data, dict):
                    continue
                command     = cmd_data.get("command")
                ts          = cmd_data.get("timestamp", 0)
                acknowledged = cmd_data.get("acknowledged", False)

                if acknowledged or command is None:
                    continue

                age = now - (ts / 1000 if ts > 1e10 else ts)  # handle ms vs s timestamps
                if age > STALE_COMMAND_TTL_S:
                    log.info("Stale command discarded — machine %s age=%ds", machine_id, int(age))
                    commands_ref.child(machine_id).update({
                        "command":      None,
                        "acknowledged": True,
                        "ack_at":       now,
                        "discarded":    True,
                    })
                    continue

                log.info("Forwarding command '%s' to machine %s", command, machine_id)
                payload = {
                    "cmd":       command,
                    "issued_by": cmd_data.get("issued_by", ""),
                    "ts":        ts,
                }
                # Pass OTA-specific fields through so firmware can call esp_https_ota
                if command == "ota":
                    payload["url"]     = cmd_data.get("url", "")
                    payload["version"] = cmd_data.get("version", "")
                mqtt_publish(
                    f"mms/nodes/{machine_id}/command",
                    payload,
                    qos=1,
                    retain=False,
                )
        except Exception:
            log.exception("on_commands_change failed")

    commands_ref.listen(on_commands_change)
    log.info("RTDB command listener active")


# ---------------------------------------------------------------------------
# Firestore listeners: machines config + revocation → MQTT
# ---------------------------------------------------------------------------
def setup_firestore_config_listener():
    """Watch /machines/* and push retained config messages per node."""
    machines_col = _fs_client.collection("machines")

    def on_machines_snapshot(col_snapshot, changes, read_time):
        try:
            for change in changes:
                if change.type.name in ("ADDED", "MODIFIED"):
                    machine_id = change.document.id
                    data = change.document.to_dict()
                    config_payload = {
                        "name":               data.get("name", f"Machine {machine_id}"),
                        "session_limit_min":  data.get("session_limit_minutes", 15),
                        "relay_active_high":  data.get("relay_active_high", True),
                        "machine_active":     data.get("machine_active", True),
                    }
                    mqtt_publish(
                        f"mms/nodes/{machine_id}/config",
                        config_payload,
                        qos=1,
                        retain=True,
                    )
                    log.info("Config pushed to machine %s: %s", machine_id, config_payload)
        except Exception:
            log.exception("on_machines_snapshot failed")

    unsubscribe = machines_col.on_snapshot(on_machines_snapshot)
    _listeners.append(unsubscribe)
    log.info("Firestore machines config listener active")


def setup_firestore_revocation_listener():
    """Watch /revoked/* and push compact revocation list to MQTT (retained)."""
    revoked_col = _fs_client.collection("revoked")

    def rebuild_and_publish(_col_snapshot=None, _changes=None, _read_time=None):
        try:
            all_revoked = revoked_col.get()
            uids = [doc.id for doc in all_revoked]
            payload = {"uids": uids, "updated_at": int(time.time())}
            mqtt_publish("mms/all/revoked", payload, qos=1, retain=True)
            # Mirror to RTDB for web app access
            _rtdb_ref.child("revoked").set(payload)
            log.info("Revocation list published — %d UIDs", len(uids))
        except Exception:
            log.exception("rebuild_and_publish revocation failed")

    unsubscribe = revoked_col.on_snapshot(rebuild_and_publish)
    _listeners.append(unsubscribe)
    log.info("Firestore revocation listener active")


# ---------------------------------------------------------------------------
# RTDB listener: /mms/ota → broadcast OTA command to all machines
# ---------------------------------------------------------------------------
def setup_rtdb_ota_listener():
    """
    Watch /mms/ota for OTA trigger writes from the super-admin web UI.
    When target_version + firmware_url appear, publish {cmd:"ota", url, version}
    to every machine's command topic so all nodes update simultaneously.
    """
    ota_ref = _rtdb_ref.child("ota")

    def on_ota_change(event):
        try:
            if event.data is None:
                return
            data = event.data
            if not isinstance(data, dict):
                return

            firmware_url   = data.get("firmware_url", "")
            target_version = data.get("target_version", "")
            released_at    = data.get("released_at", 0)

            if not firmware_url or not target_version:
                log.debug("OTA RTDB entry incomplete — skipping (url=%s version=%s)",
                          firmware_url, target_version)
                return

            # Fetch all machine IDs from Firestore so we know who to notify
            machines = _fs_client.collection("machines").get()
            machine_ids = [doc.id for doc in machines]

            now = int(time.time())
            for machine_id in machine_ids:
                mqtt_publish(
                    f"mms/nodes/{machine_id}/command",
                    {
                        "cmd":       "ota",
                        "url":       firmware_url,
                        "version":   target_version,
                        "ts":        now,
                        "issued_by": "ota_system",
                    },
                    qos=1,
                    retain=False,
                )
                log.info("OTA command sent → machine %s  version=%s", machine_id, target_version)

        except Exception:
            log.exception("on_ota_change failed")

    ota_ref.listen(on_ota_change)
    log.info("RTDB OTA listener active")


# ---------------------------------------------------------------------------
# Periodic: stale command cleanup sweep
# ---------------------------------------------------------------------------
def command_cleanup_loop():
    """Every 10 minutes, null any unacknowledged RTDB command older than 10 min."""
    while True:
        time.sleep(COMMAND_CLEANUP_INTERVAL_S)
        try:
            commands_snap = _rtdb_ref.child("commands").get()
            if not commands_snap:
                continue
            now = int(time.time())
            for machine_id, cmd_data in commands_snap.items():
                if not isinstance(cmd_data, dict):
                    continue
                if cmd_data.get("acknowledged"):
                    continue
                ts = cmd_data.get("timestamp", 0)
                age = now - (ts / 1000 if ts > 1e10 else ts)
                if age > COMMAND_CLEANUP_INTERVAL_S:
                    _rtdb_ref.child(f"commands/{machine_id}").update({
                        "command":      None,
                        "acknowledged": True,
                        "ack_at":       now,
                        "discarded":    True,
                    })
                    log.info("Cleanup: stale command nulled — machine %s", machine_id)
        except Exception:
            log.exception("command_cleanup_loop error")


# ---------------------------------------------------------------------------
# Periodic: systemd watchdog ping
# ---------------------------------------------------------------------------
def watchdog_loop():
    while True:
        time.sleep(WATCHDOG_INTERVAL_S)
        if HAVE_SYSTEMD:
            sd_daemon.notify("WATCHDOG=1")


# ---------------------------------------------------------------------------
# Health check HTTP endpoint
# ---------------------------------------------------------------------------
def health_server():
    from http.server import BaseHTTPRequestHandler, HTTPServer
    import json as _json

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/health":
                body = _json.dumps({
                    "mqtt_connected": _mqtt_connected,
                    "fb_connected":   _fb_connected,
                    "uptime_s":       int(time.time() - _start_time),
                }).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(body)
            else:
                self.send_response(404)
                self.end_headers()

        def log_message(self, fmt, *args):
            pass  # suppress access logs

    server = HTTPServer(("", HEALTH_PORT), Handler)
    server.serve_forever()


# ---------------------------------------------------------------------------
# MQTT reconnect loop (exponential backoff)
# ---------------------------------------------------------------------------
def mqtt_reconnect_loop(client: mqtt.Client):
    delay = 1
    while True:
        if not _mqtt_connected:
            try:
                log.info("Attempting MQTT reconnect (delay=%ds)...", delay)
                client.reconnect()
                delay = 1
            except Exception as e:
                log.warning("MQTT reconnect failed: %s — retrying in %ds", e, delay)
                delay = min(delay * 2, 60)
        time.sleep(delay)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    global _mqtt_client

    log.info("MMS Bridge starting...")

    init_firebase()

    # MQTT setup
    _mqtt_client = mqtt.Client(client_id="bridge", clean_session=True)
    _mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)
    _mqtt_client.on_connect    = on_connect
    _mqtt_client.on_disconnect = on_disconnect
    _mqtt_client.on_message    = on_message
    _mqtt_client.on_publish    = on_publish

    # Set bridge LWT (so nodes know if bridge died)
    _mqtt_client.will_set("mms/bridge/status", json.dumps({"state": "offline"}), qos=1, retain=True)

    _mqtt_client.connect(MQTT_BROKER, MQTT_PORT, keepalive=MQTT_KEEPALIVE)
    _mqtt_client.loop_start()

    # Wait for initial MQTT connection
    for _ in range(20):
        if _mqtt_connected:
            break
        time.sleep(0.5)

    # Announce bridge online
    mqtt_publish("mms/bridge/status", {"state": "online", "ts": int(time.time())}, retain=True)

    # Set up Firebase listeners
    setup_rtdb_command_listener()
    setup_rtdb_ota_listener()
    setup_firestore_config_listener()
    setup_firestore_revocation_listener()

    # Background threads
    threading.Thread(target=command_cleanup_loop, daemon=True).start()
    threading.Thread(target=watchdog_loop, daemon=True).start()
    threading.Thread(target=health_server, daemon=True).start()
    threading.Thread(target=mqtt_reconnect_loop, args=(_mqtt_client,), daemon=True).start()

    if HAVE_SYSTEMD:
        sd_daemon.notify("READY=1")
        log.info("systemd notified: READY")

    log.info("MMS Bridge running — MQTT=%s:%d Firebase=%s", MQTT_BROKER, MQTT_PORT, RTDB_URL)

    # Main thread: keep alive, handle signals
    def shutdown(signum, frame):
        log.info("Shutdown signal received — stopping bridge")
        mqtt_publish("mms/bridge/status", {"state": "offline"}, retain=True)
        _mqtt_client.loop_stop()
        _mqtt_client.disconnect()
        for unsubscribe in _listeners:
            try:
                unsubscribe()
            except Exception:
                pass
        sys.exit(0)

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    while True:
        time.sleep(1)


if __name__ == "__main__":
    main()
