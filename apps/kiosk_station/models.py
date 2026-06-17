"""
MMS V2 — Kiosk data models and cryptographic helpers.

Card schema V2 (Sector 1, blocks 4-6):
  Block 4  [0..8]  Roll number ASCII (9 bytes, e.g. "220002123")
           [9]     Schema version (0x02)
           [10..15] Reserved 0x00

  Block 5  [0..7]  PIN hash: HMAC-SHA256(master_key || uid_hex, pin_string)[0:8]
           [8..11] Machine permissions (uint32 little-endian)
           [12..15] Reserved 0x00

  Block 6  All 0x00 (reserved)

Sector key derivation (per-card):
  sector_key = HMAC-SHA256(master_key, uid_hex)[0:6]
"""

import hmac
import hashlib
import struct
from dataclasses import dataclass
from typing import Optional

from secrets import MASTER_KEY


# ---------------------------------------------------------------------------
# Cryptographic helpers
# ---------------------------------------------------------------------------

def derive_sector_key(uid_bytes: bytes) -> bytes:
    """Derive the 6-byte MIFARE sector key for this card's UID."""
    uid_hex = uid_bytes.hex().upper()
    digest = hmac.new(MASTER_KEY, uid_hex.encode(), hashlib.sha256).digest()
    return digest[:6]


def hash_pin(pin: str, uid_bytes: bytes) -> bytes:
    """
    Compute the 8-byte PIN hash stored in Block 5.
    hash = HMAC-SHA256(master_key || uid_hex, pin)[0:8]
    """
    uid_hex = uid_bytes.hex().upper()
    key = MASTER_KEY + uid_hex.encode()
    digest = hmac.new(key, pin.encode(), hashlib.sha256).digest()
    return digest[:8]


def verify_pin(pin: str, uid_bytes: bytes, stored_hash_hex: str) -> bool:
    expected = hash_pin(pin, uid_bytes)
    stored   = bytes.fromhex(stored_hash_hex)
    return hmac.compare_digest(expected, stored)


# ---------------------------------------------------------------------------
# Card data packing
# ---------------------------------------------------------------------------

def pack_block4(roll_number: str) -> bytes:
    """Pack identity block: 9-byte roll + 1 schema version byte + 6 zero bytes."""
    block = bytearray(16)
    roll_bytes = roll_number.encode("ascii")
    if len(roll_bytes) > 9:
        raise ValueError(f"Roll number too long: {roll_number!r}")
    block[:len(roll_bytes)] = roll_bytes
    block[9] = 0x02  # schema version V2
    return bytes(block)


def pack_block5(pin: str, uid_bytes: bytes, perm_mask: int) -> bytes:
    """Pack auth + permissions block."""
    if not (0 <= perm_mask <= 0xFFFFFFFF):
        raise ValueError(f"perm_mask out of range: {perm_mask}")
    block = bytearray(16)
    pin_hash = hash_pin(pin, uid_bytes)
    block[0:8] = pin_hash
    struct.pack_into("<I", block, 8, perm_mask)  # uint32 little-endian at offset 8
    return bytes(block)


# ---------------------------------------------------------------------------
# Permission bitmask helpers
# ---------------------------------------------------------------------------

def machine_ids_to_mask(machine_ids: list[int]) -> int:
    mask = 0
    for mid in machine_ids:
        if not (1 <= mid <= 32):
            raise ValueError(f"Machine ID out of range: {mid}")
        mask |= (1 << (mid - 1))
    return mask


def mask_to_machine_ids(mask: int) -> list[int]:
    return [i + 1 for i in range(32) if (mask >> i) & 1]


# ---------------------------------------------------------------------------
# Serial command to ESP32 station writer
# ---------------------------------------------------------------------------

@dataclass
class CardWriteCommand:
    roll_number:  str
    perm_mask:    int
    pin:          str

    def to_serial_json(self, uid_bytes: bytes) -> str:
        import json
        pin_hash_hex = hash_pin(self.pin, uid_bytes).hex()
        return json.dumps({
            "roll":      self.roll_number,
            "perm_mask": self.perm_mask,
            "pin_hash":  pin_hash_hex,
            "uid":       uid_bytes.hex().upper(),
        })
