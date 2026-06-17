#!/bin/bash
# MMS V2 RPi Bridge Setup Script
# Run once on a fresh Raspberry Pi after cloning the repo.
# Usage: sudo bash setup.sh

set -e

echo "=== MMS V2 Bridge Setup ==="

# System deps
apt-get update -q
apt-get install -y python3-pip mosquitto mosquitto-clients python3-systemd

# Python deps
pip3 install -r requirements.txt

# Working directory
mkdir -p /home/pi/mms
cp bridge.py /home/pi/mms/
cp .env.template /home/pi/mms/.env
chown -R pi:pi /home/pi/mms

echo ""
echo "NEXT STEPS:"
echo "  1. Copy service-account.json to /home/pi/mms/service-account.json"
echo "  2. Edit /home/pi/mms/.env with your Firebase RTDB URL and MQTT passwords"
echo "  3. Run: python3 generate_mqtt_creds.py --machines 15"
echo "  4. cp mosquitto.conf /etc/mosquitto/conf.d/mms.conf"
echo "  5. systemctl restart mosquitto"
echo "  6. cp mms-bridge.service /etc/systemd/system/"
echo "  7. systemctl enable mms-bridge && systemctl start mms-bridge"
echo "  8. journalctl -u mms-bridge -f   # to watch logs"
