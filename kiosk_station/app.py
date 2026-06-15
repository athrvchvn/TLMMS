"""
MMS V2 — Kiosk Station Flask Application
Card issuance flow: roll entry → PIN setup → machine selection → card write → Firestore update.
"""

import logging
from flask import Flask, jsonify, render_template, request
from flask_cors import CORS

import firebase_config
import rfid_handler
from models import (
    CardWriteCommand,
    derive_sector_key,
    hash_pin,
    machine_ids_to_mask,
    verify_pin,
)
from config import MACHINE_IDS

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
log = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)

# Session state (single-user kiosk — no concurrent sessions)
_session: dict = {}


def _reset_session():
    global _session
    _session = {}


# ---------------------------------------------------------------------------
# Startup
# ---------------------------------------------------------------------------
firebase_config.init()
rfid_handler.open_port()


# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/check_user", methods=["POST"])
def check_user():
    """Step 1: Verify roll number exists in Firestore."""
    data = request.get_json()
    roll = data.get("roll_number", "").strip()

    if not roll or len(roll) != 9 or not roll.isdigit():
        return jsonify({"success": False, "error": "Invalid roll number (must be 9 digits)"}), 400

    user = firebase_config.get_user_by_roll(roll)
    if not user:
        return jsonify({"success": False, "error": "Roll number not found. Contact admin."}), 404

    _reset_session()
    _session["roll"]      = roll
    _session["user_name"] = user.get("name", "")
    _session["email"]     = user.get("email", "")
    _session["machines"]  = firebase_config.get_all_machines()

    has_card = bool(user.get("card_uid"))
    return jsonify({
        "success":  True,
        "name":     _session["user_name"],
        "has_card": has_card,
        "machines": _session["machines"],
    })


@app.route("/api/read_uid", methods=["POST"])
def read_uid():
    """Step 2a: Ask ESP32 to scan the card and return its UID."""
    uid = rfid_handler.read_card_uid()
    if not uid:
        return jsonify({"success": False, "error": "No card detected. Place card on reader."}), 408

    _session["uid_hex"] = uid
    return jsonify({"success": True, "uid": uid})


@app.route("/api/begin_write", methods=["POST"])
def begin_write():
    """
    Step 3: Receive PIN + selected machine IDs, trigger card write.
    Expects: {pin: "1234", machine_ids: [1, 3, 5]}
    """
    data = request.get_json()
    pin         = str(data.get("pin", "")).strip()
    machine_ids = data.get("machine_ids", [])

    if not pin or len(pin) != 4 or not pin.isdigit():
        return jsonify({"success": False, "error": "PIN must be exactly 4 digits"}), 400
    if not machine_ids:
        return jsonify({"success": False, "error": "Select at least one machine"}), 400
    if "roll" not in _session:
        return jsonify({"success": False, "error": "Session expired — start over"}), 400
    if "uid_hex" not in _session:
        return jsonify({"success": False, "error": "Card UID not read — scan card first"}), 400

    roll    = _session["roll"]
    uid_hex = _session["uid_hex"]
    uid_bytes = bytes.fromhex(uid_hex)

    perm_mask    = machine_ids_to_mask([int(m) for m in machine_ids])
    pin_hash_hex = hash_pin(pin, uid_bytes).hex()

    success, message = rfid_handler.write_card(roll, perm_mask, pin_hash_hex, uid_hex)

    if success:
        # Update Firestore
        firebase_config.update_card_data(roll, uid_hex, perm_mask, pin_hash_hex)
        _session["perm_mask"]    = perm_mask
        _session["pin_hash_hex"] = pin_hash_hex
        log.info("Card issued — roll=%s uid=%s perm=0x%08X", roll, uid_hex, perm_mask)

    return jsonify({"success": success, "message": message})


@app.route("/api/verify_pin", methods=["POST"])
def api_verify_pin():
    """
    Verify PIN against existing card data (for re-issue flow).
    Expects: {roll_number, pin, uid_hex}
    """
    data = request.get_json()
    roll    = data.get("roll_number", "").strip()
    pin     = str(data.get("pin", "")).strip()
    uid_hex = data.get("uid_hex", "").strip()

    user = firebase_config.get_user_by_roll(roll)
    if not user:
        return jsonify({"success": False, "error": "User not found"}), 404

    stored_hash = user.get("pin_hash", "")
    if not stored_hash:
        return jsonify({"success": False, "error": "No PIN set for this user"}), 400

    uid_bytes = bytes.fromhex(uid_hex)
    if verify_pin(pin, uid_bytes, stored_hash):
        return jsonify({"success": True})
    return jsonify({"success": False, "error": "Incorrect PIN"}), 401


@app.route("/api/reset", methods=["POST"])
def reset_session():
    _reset_session()
    return jsonify({"success": True})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
