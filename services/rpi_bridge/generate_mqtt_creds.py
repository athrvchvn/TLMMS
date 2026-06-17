#!/usr/bin/env python3
"""
MMS V2 — MQTT credential and ACL generator.
Generates:
  - /etc/mosquitto/passwd entries (via mosquitto_passwd)
  - /etc/mosquitto/mms_acl.conf

Run as root or with sudo.
Usage: python3 generate_mqtt_creds.py --machines 15 --passwd-file /etc/mosquitto/passwd
"""

import argparse
import os
import secrets
import subprocess
import sys

ACL_TEMPLATE = """\
# MMS V2 MQTT ACLs — auto-generated, do not edit manually
# Regenerate: python3 generate_mqtt_creds.py

# Bridge: full access to all MMS topics
user bridge
topic readwrite mms/#

"""

NODE_ACL_TEMPLATE = """\
user node_{id}
topic write mms/nodes/{id}/status
topic write mms/nodes/{id}/event
topic write mms/nodes/{id}/heartbeat
topic read  mms/nodes/{id}/command
topic read  mms/nodes/{id}/config
topic read  mms/all/revoked

"""


def generate_password(length: int = 24) -> str:
    return secrets.token_urlsafe(length)


def add_mosquitto_user(passwd_file: str, username: str, password: str, create_new: bool = False):
    flag = "-c" if create_new else "-b"
    try:
        result = subprocess.run(
            ["mosquitto_passwd", flag, passwd_file, username, password],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            print(f"  ERROR: {result.stderr.strip()}", file=sys.stderr)
            return False
        return True
    except FileNotFoundError:
        print("ERROR: mosquitto_passwd not found. Install mosquitto-clients.", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="Generate MMS MQTT credentials and ACLs")
    parser.add_argument("--machines", type=int, default=15, help="Number of machine nodes (default: 15)")
    parser.add_argument("--passwd-file", default="/etc/mosquitto/passwd", help="Mosquitto password file path")
    parser.add_argument("--acl-file", default="/etc/mosquitto/mms_acl.conf", help="ACL output file path")
    parser.add_argument("--creds-out", default="mqtt_credentials.txt", help="Plain-text credentials output (for secrets.h provisioning)")
    args = parser.parse_args()

    print(f"Generating MQTT credentials for {args.machines} nodes...")

    creds = {}

    # Bridge password
    bridge_pass = generate_password()
    creds["bridge"] = bridge_pass

    # Node passwords
    for i in range(1, args.machines + 1):
        creds[f"node_{i}"] = generate_password()

    # Write Mosquitto password file
    print(f"\nWriting password file: {args.passwd_file}")
    first = True
    for username, password in creds.items():
        ok = add_mosquitto_user(args.passwd_file, username, password, create_new=first)
        first = False
        status = "OK" if ok else "FAILED"
        print(f"  {username}: {status}")

    # Write ACL file
    print(f"\nWriting ACL file: {args.acl_file}")
    acl_content = ACL_TEMPLATE
    for i in range(1, args.machines + 1):
        acl_content += NODE_ACL_TEMPLATE.format(id=i)

    os.makedirs(os.path.dirname(args.acl_file) or ".", exist_ok=True)
    with open(args.acl_file, "w") as f:
        f.write(acl_content)
    print(f"  ACL written for {args.machines} nodes")

    # Write plain-text credentials for provisioning
    print(f"\nWriting credentials to: {args.creds_out}")
    with open(args.creds_out, "w") as f:
        f.write("# MMS V2 MQTT Credentials\n")
        f.write("# Copy node_{id} password into each node's secrets.h\n\n")
        for username, password in creds.items():
            f.write(f"{username}: {password}\n")
    os.chmod(args.creds_out, 0o600)

    print(f"\nDone. Update .env with: MQTT_PASS={creds['bridge']}")
    print(f"Reload Mosquitto: sudo systemctl reload mosquitto")


if __name__ == "__main__":
    main()
