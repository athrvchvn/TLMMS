"""
MMS V2 — Serial communication handler for ESP32 station writer.

Protocol:
  Host → ESP32: JSON line {"roll":"220002123","perm_mask":15,"pin_hash":"aabbccdd...","uid":"A1B2C3D4"}
  ESP32 → Host: Lines containing "CARD_UID:", "WRITE_SUCCESS", "WRITE_FAILED", "ERR:..."
"""

import json
import logging
import time
import serial
from config import RFID_PORT, RFID_BAUDRATE, RFID_TIMEOUT_S

log = logging.getLogger(__name__)

_serial: serial.Serial | None = None


def open_port() -> bool:
    global _serial
    try:
        _serial = serial.Serial(RFID_PORT, RFID_BAUDRATE, timeout=1)
        time.sleep(2)  # allow ESP32 to boot after DTR reset
        _serial.flushInput()
        log.info("Serial port %s open", RFID_PORT)
        return True
    except serial.SerialException as e:
        log.error("Cannot open %s: %s", RFID_PORT, e)
        return False


def close_port():
    if _serial and _serial.is_open:
        _serial.close()


def read_card_uid(timeout_s: int = RFID_TIMEOUT_S) -> str | None:
    """
    Ask ESP32 to scan for a card and return its UID.
    Send command "READ_UID\n", wait for "CARD_UID:<hex>" response.
    """
    if not _serial or not _serial.is_open:
        return None
    _serial.write(b"READ_UID\n")
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        line = _serial.readline().decode("utf-8", errors="ignore").strip()
        if line.startswith("CARD_UID:"):
            uid = line[len("CARD_UID:"):].strip()
            log.info("Card UID read: %s", uid)
            return uid
        if line.startswith("ERR:"):
            log.warning("ESP32 error: %s", line)
    return None


def write_card(roll_number: str, perm_mask: int, pin_hash_hex: str, uid_hex: str,
               timeout_s: int = RFID_TIMEOUT_S) -> tuple[bool, str]:
    """
    Send card write command to ESP32 station writer.
    Returns (success, message).
    """
    if not _serial or not _serial.is_open:
        return False, "Serial port not open"

    payload = json.dumps({
        "roll":      roll_number,
        "perm_mask": perm_mask,
        "pin_hash":  pin_hash_hex,
        "uid":       uid_hex,
    }) + "\n"

    _serial.write(payload.encode("utf-8"))
    log.info("Card write command sent: roll=%s perm=0x%08X", roll_number, perm_mask)

    deadline = time.time() + timeout_s
    while time.time() < deadline:
        line = _serial.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue
        log.debug("ESP32: %s", line)
        if line == "WRITE_SUCCESS":
            return True, "Card written successfully"
        if line == "WRITE_FAILED":
            return False, "Card write failed — check card placement"
        if line.startswith("ERR:"):
            return False, line

    return False, "Timeout — no card presented"
