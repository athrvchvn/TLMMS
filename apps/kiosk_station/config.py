RFID_PORT      = "/dev/ttyUSB0"
RFID_BAUDRATE  = 115200
RFID_TIMEOUT_S = 30        # seconds to wait for card placement after initiating write
CARD_SCHEMA_VERSION = 0x02

# CANONICAL MACHINE ID TABLE — must match:
#   - access_node/config.h  MACHINE_ID per node
#   - Firestore /machines/{id} document IDs (seeded by scripts/seed_machines.py)
#
# To add a machine: add an entry here AND create the Firestore document.
# Changing an existing ID invalidates all cards issued for that machine.
MACHINE_IDS = {
    "Creality 3D Printer":    1,
    "Fracktal Laser Cutter":  2,
    "Soldering Station 1":    3,
    "Soldering Station 2":    4,
    "Bosche Grinder":         5,
}
