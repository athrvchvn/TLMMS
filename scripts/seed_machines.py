#!/usr/bin/env python3
"""
Seed Firestore /machines collection with initial machine documents.

Run this ONCE before deploying any nodes or issuing cards. The machine IDs
defined here are the canonical source of truth — they MUST match:
  - kiosk_station/config.py  MACHINE_IDS dict values
  - access_node/config.h     MACHINE_ID per node (one row per physical node)

Existing documents are skipped (idempotent). To update an existing machine
use the web app admin dashboard after deployment.

Usage:
    export GOOGLE_APPLICATION_CREDENTIALS=/path/to/service-account.json
    python3 seed_machines.py [--dry-run]

Requirements:
    pip install firebase-admin
"""

import argparse
import os
import sys

try:
    import firebase_admin
    from firebase_admin import credentials, firestore
except ImportError:
    print("ERROR: firebase-admin not installed. Run: pip install firebase-admin")
    sys.exit(1)


# ── Canonical machine definitions ────────────────────────────────────────────
# Edit this list to match the physical lab setup.
# Do NOT change an ID after cards have been issued — it invalidates all
# permissions for that machine and requires card re-issue.
MACHINES = [
    {
        "id":                    "1",
        "name":                  "Creality 3D Printer",
        "session_limit_minutes": 60,
        "relay_active_high":     True,
        "machine_active":        True,
        "location":              "Zone A",
    },
    {
        "id":                    "2",
        "name":                  "Fracktal Laser Cutter",
        "session_limit_minutes": 30,
        "relay_active_high":     True,
        "machine_active":        True,
        "location":              "Zone B",
    },
    {
        "id":                    "3",
        "name":                  "Soldering Station 1",
        "session_limit_minutes": 15,
        "relay_active_high":     True,
        "machine_active":        True,
        "location":              "Zone C",
    },
    {
        "id":                    "4",
        "name":                  "Soldering Station 2",
        "session_limit_minutes": 15,
        "relay_active_high":     True,
        "machine_active":        True,
        "location":              "Zone C",
    },
    {
        "id":                    "5",
        "name":                  "Bosche Grinder",
        "session_limit_minutes": 20,
        "relay_active_high":     True,
        "machine_active":        True,
        "location":              "Zone D",
    },
]
# ─────────────────────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="Seed Firestore /machines collection")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print what would be written without touching Firestore")
    args = parser.parse_args()

    sa_path = os.environ.get("GOOGLE_APPLICATION_CREDENTIALS")
    if not sa_path:
        print("ERROR: GOOGLE_APPLICATION_CREDENTIALS not set")
        sys.exit(1)
    if not os.path.exists(sa_path):
        print(f"ERROR: service account file not found: {sa_path}")
        sys.exit(1)

    if not args.dry_run:
        cred = credentials.Certificate(sa_path)
        firebase_admin.initialize_app(cred)
        db = firestore.client()

    print(f"{'[DRY RUN] ' if args.dry_run else ''}Seeding {len(MACHINES)} machines...\n")

    created = 0
    skipped = 0
    for m in MACHINES:
        machine_id = m["id"]
        data = {k: v for k, v in m.items() if k != "id"}

        if args.dry_run:
            print(f"  WOULD CREATE /machines/{machine_id}: {data}")
            created += 1
            continue

        ref = db.collection("machines").document(machine_id)
        if ref.get().exists:
            print(f"  [{machine_id}] {data['name']:30s} — already exists, skipped")
            skipped += 1
        else:
            ref.set(data)
            print(f"  [{machine_id}] {data['name']:30s} — CREATED")
            created += 1

    print(f"\n{'Would create' if args.dry_run else 'Created'}: {created}   Skipped: {skipped}")

    if not args.dry_run and created > 0:
        print("\nNext steps:")
        print("  1. Restart rpi_bridge — it will push config to nodes via MQTT")
        print("  2. Flash each node with the matching MACHINE_ID from this list")
        print("  3. Issue cards via the kiosk — machine names will appear in the dropdown")


if __name__ == "__main__":
    main()
