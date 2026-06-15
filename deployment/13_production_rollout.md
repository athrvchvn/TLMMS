# Section 13 — Production Rollout

← [Integration Testing](12_integration_testing.md) | → [Maintenance Handbook](14_maintenance_handbook.md)

---

## Overview

Production rollout deploys validated nodes to live machines in the lab. It uses a phased approach: one machine first, then five, then all fifteen. This limits blast radius if something goes wrong.

**Prerequisites before any machine goes live:**
- [ ] Raspberry Pi deployed and running (Section 07)
- [ ] Firebase deployed, rules deployed, machines seeded (Section 08)
- [ ] Web app deployed and admin account created (Section 09)
- [ ] At least 5 cards issued for frequent lab users (Section 10)
- [ ] All nodes assembled, validated, and provisioned (Sections 05, 06, 11)
- [ ] Integration tests passed on all nodes (Section 12)

---

## Phase 0 — Pre-Rollout Checklist

Run these verification commands before deploying any node to a live machine:

```bash
# 1. Bridge is running and healthy
curl http://192.168.0.10:8080/health
# Expected: {"mqtt_connected": true, "fb_connected": true, "uptime_s": N}

# 2. Mosquitto is accepting connections
mosquitto_pub -h 192.168.0.10 -t mms/test -m "ping"
# Should not error

# 3. Firebase Firestore is reachable
firebase firestore:get /machines/1
# Should show Creality 3D Printer document

# 4. All 5 machines are in Firestore
firebase firestore:get /machines/1
firebase firestore:get /machines/2
firebase firestore:get /machines/3
firebase firestore:get /machines/4
firebase firestore:get /machines/5
# All should return documents

# 5. Web app loads correctly
# Open https://your-project.web.app → login → dashboard shows 5 machines
```

If any check fails, fix it before proceeding.

---

## Phase 1 — Pilot: One Machine (Day 1)

**Choose the lowest-risk machine for the pilot.** A soldering station (ID 3 or 4) is ideal: it draws less current than the laser cutter or grinder, and sessions are short (15 min), so any issue is discovered quickly.

### Steps

1. **Inform lab users** that one machine is being upgraded. Expect brief downtime.

2. **Power off the machine** at the wall switch.

3. **Install the node enclosure** on or near the machine:
   - Mount enclosure securely
   - Route AC cable through cable gland to HiLink input
   - Connect relay COM and NO in series with the machine's AC power line
   - Attach the HC89 sensor so cards can be inserted from outside the enclosure

4. **Power on the node** (leave machine in relay-off state):
   - Both OLEDs should light up
   - Serial (if monitoring): boot sequence should complete to IDLE

5. **Verify on dashboard:**
   - Machine 3 (or whichever you chose) should show state: `idle` within 10 seconds

6. **Test access with a permitted card:**
   - Insert card → relay should energise → machine should receive AC power → run briefly
   - Remove card → relay de-energises → machine loses power

7. **Tell 2–3 regular users** about the new system and demonstrate usage.

8. **Monitor for 2 hours:**
   - Watch `sudo journalctl -u mms-bridge -f` on RPi for errors
   - Watch Firestore sessions collection for correct entries
   - Check dashboard for anomalies

### Rollback for Phase 1

If anything goes wrong during pilot:
1. Power off the node
2. Reconnect the machine directly to mains (bypassing relay)
3. Machine is usable again immediately
4. Investigate the problem before re-deploying

---

## Phase 2 — Expand: Five Machines (Week 1)

After Phase 1 runs without issues for at least 24 hours, deploy to all five machines.

### Deployment Order

Deploy in ascending risk order:

| Order | Machine | Zone | Session | Notes |
|---|---|---|---|---|
| 1 | Soldering Station 1 (ID 3) | Zone C | 15 min | Already done in Phase 1 |
| 2 | Soldering Station 2 (ID 4) | Zone C | 15 min | Same zone, similar hardware |
| 3 | Bosche Grinder (ID 5) | Zone D | 20 min | Higher risk — confirm relay handles current |
| 4 | Creality 3D Printer (ID 1) | Zone A | 60 min | Long sessions — confirm timeout works |
| 5 | Fracktal Laser Cutter (ID 2) | Zone B | 30 min | Highest risk — deploy last |

### Laser Cutter Special Notes

The laser cutter requires extra attention:
- Confirm relay type: SSR (solid-state relay) is strongly recommended for the laser cutter due to higher switching frequency and noise sensitivity
- Verify `relay_active_high` config matches the SSR wiring before energising
- Test relay toggle 10 times before connecting to the cutter
- Have a lab coordinator present for the first live test

### Per-Machine Deployment Steps

For each machine beyond the pilot:
1. Schedule a 15-minute window when the machine is not in use
2. Install node (same procedure as Phase 1, steps 2–4)
3. Verify on dashboard (step 5)
4. Test with at least one permitted card (step 6)
5. Sign the node provisioning checklist (Section 11)

### Monitor Phase 2

After all 5 machines are live:
- Check dashboard twice daily for one week
- Review Firestore sessions daily — look for orphaned active sessions or unusual end_reasons
- RPi bridge logs: `sudo journalctl -u mms-bridge --since "1 day ago" | grep -i error`

---

## Rollback Procedures

### Rollback a single node

If one node fails after deployment:
```bash
# 1. Power off the node
# 2. Bypass the relay (connect machine directly to mains)
# 3. Disconnect node from lab WiFi (DHCP expiry) or disconnect Ethernet if wired
# 4. Diagnose via USB serial:
#    Connect laptop → Serial Monitor → observe boot/error output
# 5. Re-flash with known-good firmware after fixing issue
# 6. Re-run integration test (Section 12) before reinstalling
```

### Rollback the bridge

If bridge.py has a bug that causes data loss:
```bash
# Stop new bridge
sudo systemctl stop mms-bridge

# Restore previous bridge.py from git
cd /home/pi/mms
git log --oneline  # find last known-good commit
git checkout <commit-hash> -- bridge.py

# Or restore from backup
cp /home/pi/backup/bridge.py /home/pi/mms/bridge.py

# Restart
sudo systemctl start mms-bridge
sudo journalctl -u mms-bridge -f
```

### Rollback an OTA update

If a firmware OTA update causes problems (node enters error loop):
- The firmware has automatic rollback: if the new firmware crashes 3× on boot, it reverts to the previous partition automatically
- Manual rollback: flash the previous firmware version via USB

---

## Risk Mitigation

| Risk | Likelihood | Mitigation |
|---|---|---|
| Relay wired backwards (NO vs NC) | Medium | Test relay continuity with multimeter before connecting machine |
| Node loses power mid-session | Low | Relay fails to safe (OFF); session orphan cleaned on next boot |
| RPi goes down during busy period | Medium | Nodes continue offline; sessions buffered in LittleFS |
| Master key mismatch between kiosk and node | Low | Use the same key generation script; copy once, store securely |
| Student loses card during session | Low | Card removal ends session; remote stop from dashboard as fallback |
| WiFi AP unreachable | Low | Offline mode handles access; sessions buffered |
| Laser cutter control circuit noise | Low | Use SSR, add EMI filter on AC line if needed |

---

## Go/No-Go Criteria for Full Production

Before considering the rollout complete:

- [ ] All 5 machines deployed and stable for 1 week
- [ ] At least 20 real student sessions successfully logged
- [ ] At least 1 remote stop executed by an admin
- [ ] At least 1 card revocation tested end-to-end
- [ ] Bridge has been restarted at least once without data loss
- [ ] At least 1 offline session (RPi restart) successfully synced on reconnect
- [ ] No orphaned active sessions older than 2 hours in Firestore
- [ ] All lab coordinators trained on web dashboard and card issuance
- [ ] Emergency contacts and escalation path defined (see README.md)

Once all criteria are met, the system is in production.
