"""
MMS V2 — Firebase Admin SDK helpers for kiosk station.
"""

import logging
from datetime import datetime, timezone

import firebase_admin
from firebase_admin import credentials, firestore

log = logging.getLogger(__name__)

_db = None


def init():
    global _db
    if not firebase_admin._apps:
        cred = credentials.Certificate("service-account.json")
        firebase_admin.initialize_app(cred)
    _db = firestore.client()
    log.info("Firebase initialized for kiosk station")


def get_user_by_roll(roll_number: str) -> dict | None:
    doc = _db.collection("users").document(roll_number).get()
    if doc.exists:
        return doc.to_dict()
    return None


def save_user(roll_number: str, name: str, email: str) -> bool:
    """Create or update a user record (without card data — card data added after write)."""
    try:
        _db.collection("users").document(roll_number).set({
            "roll_number": roll_number,
            "name":        name,
            "email":       email,
            "is_active":   True,
        }, merge=True)
        return True
    except Exception:
        log.exception("save_user failed for %s", roll_number)
        return False


def update_card_data(roll_number: str, card_uid: str, perm_mask: int,
                     pin_hash_hex: str, issued_by: str = "kiosk") -> bool:
    """
    After a successful card write, update the user's Firestore record
    with the card UID, permissions bitmask, and PIN hash.
    """
    try:
        _db.collection("users").document(roll_number).set({
            "card_uid":            card_uid,
            "machine_permissions": perm_mask,
            "pin_hash":            pin_hash_hex,
            "issued_at":           firestore.SERVER_TIMESTAMP,
            "issued_by":           issued_by,
        }, merge=True)
        return True
    except Exception:
        log.exception("update_card_data failed for %s", roll_number)
        return False


def get_all_machines() -> list[dict]:
    """Return list of machines from Firestore (sorted by machine ID)."""
    try:
        docs = _db.collection("machines").get()
        machines = []
        for doc in docs:
            d = doc.to_dict()
            d["id"] = doc.id
            machines.append(d)
        return sorted(machines, key=lambda x: int(x.get("id", 0)))
    except Exception:
        log.exception("get_all_machines failed")
        return []
