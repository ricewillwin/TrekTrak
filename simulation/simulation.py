#!/usr/bin/env python3
"""
simulate_day.py — Generates and publishes 24 hours of sensor data (96 reports
at 15-minute intervals) to an MQTT broker, matching the ESP32 Wokwi sketch
payload format.

Traffic model:
  - Overnight (00:00–06:00): near-zero activity
  - Morning ramp (06:00–08:00): rising
  - Morning rush (08:00–09:30): peak
  - Midday (09:30–11:30): moderate
  - Lunch rush (11:30–13:00): second peak
  - Afternoon (13:00–16:30): moderate
  - Evening rush (16:30–18:30): peak
  - Evening taper (18:30–21:00): declining
  - Late night (21:00–00:00): low

Usage:
  pip install paho-mqtt
  python simulation.py                   # real-time (15 min between publishes)
  python simulation.py --fast            # 1 second between publishes
  python simulation.py --instant         # no delay, dump all at once
  python simulation.py --continuous      # loop forever, one day at a time
  python simulation.py --days 7          # run 7 days then stop
  python simulation.py --broker test.mosquitto.org
"""

import argparse
import csv
import json
import math
import os
import random
import time
from datetime import datetime, timedelta, timezone

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Install paho-mqtt:  pip install paho-mqtt")
    raise SystemExit(1)


# ── Configuration defaults ────────────────────────────────────────────────
BROKER       = "localhost"
PORT         = 1883
TOPIC        = "trektrak/sensor/report"
TOPIC_LOG    = "trektrak/sensor/log"
GPS_LAT      = 42.504800
GPS_LON      = -71.195600
REPORT_EVERY = timedelta(minutes=15)
SIM_START    = datetime.now(timezone.utc).replace(
                   hour=0, minute=0, second=0, microsecond=0)


# ── Traffic intensity model ───────────────────────────────────────────────
def traffic_intensity(hour: float) -> float:
    """Return a 0.0–1.0 intensity value for a given fractional hour of day.

    Built from three overlapping gaussian peaks (morning rush, lunch, evening
    rush) plus a small constant baseline.
    """
    def gauss(x, mu, sigma, amp):
        return amp * math.exp(-0.5 * ((x - mu) / sigma) ** 2)

    base = 0.02                         # overnight baseline
    morning = gauss(hour, 8.5, 0.9, 0.9)
    lunch   = gauss(hour, 12.25, 0.8, 0.65)
    evening = gauss(hour, 17.5, 1.0, 1.0)

    return min(base + morning + lunch + evening, 1.0)


# ── Sensor generation ────────────────────────────────────────────────────
def generate_report(seq: int, ts: datetime, intensity: float) -> dict:
    """Produce one 15-minute report window dict.

    Args:
        seq:        sequence number (1-based)
        ts:         timestamp at end of this window
        intensity:  0.0–1.0 traffic level from the model
    """
    # Ultrasonic: detection count scales with intensity
    #   Peak window ≈ 12–18 detections, quiet ≈ 0–1
    us_mean = intensity * 15
    us_detections = max(0, int(random.gauss(us_mean, us_mean * 0.25 + 0.3)))

    # Presence duration: each detection lasts 2–12 s on average, longer at
    # lower intensity (people lingering) vs high intensity (flowing traffic)
    if us_detections > 0:
        avg_dwell = random.uniform(2.0, 5.0) + (1 - intensity) * 4
        us_presence = round(sum(
            max(0.5, random.gauss(avg_dwell, avg_dwell * 0.4))
            for _ in range(us_detections)
        ), 1)
    else:
        us_presence = 0.0

    # PIR: wider FOV so it picks up more events than the narrow ultrasonic
    #   Roughly 1.2–2× the ultrasonic count, plus occasional extra triggers
    pir_extra = max(0, int(random.gauss(intensity * 6, 2)))
    pir_triggers = us_detections + pir_extra

    # GPS: fixed position with tiny simulated jitter (±0.00001°, ~1 m)
    lat = round(GPS_LAT + random.uniform(-0.00001, 0.00001), 6)
    lon = round(GPS_LON + random.uniform(-0.00001, 0.00001), 6)

    uptime_sec = seq * int(REPORT_EVERY.total_seconds())

    return {
        "seq":        seq,
        "gps": {
            "lat":  lat,
            "lon":  lon,
            "time": ts.strftime("%H:%M:%S") + " UTC"
        },
        "ultrasonic": {
            "detections":        us_detections,
            "total_presence_sec": us_presence
        },
        "pir": {
            "triggers": pir_triggers
        },
        "uptime_sec": uptime_sec
    }


# ── MQTT helpers ──────────────────────────────────────────────────────────
def connect_mqtt(broker: str, port: int) -> mqtt.Client:
    client_id = f"DaySim-{random.randint(0, 0xFFFF):04x}"
    client = mqtt.Client(
        client_id=client_id,
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2
    )

    def on_connect(client, userdata, flags, rc, properties=None):
        if rc == 0:
            print(f"[MQTT] Connected to {broker}:{port} as {client_id}")
        else:
            print(f"[MQTT] Connection failed (rc={rc})")

    client.on_connect = on_connect
    client.connect(broker, port, keepalive=60)
    client.loop_start()
    return client


# ── Main ──────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="Simulate 24 h of sensor data and publish to MQTT")
    parser.add_argument("--broker", default=BROKER,
                        help=f"MQTT broker hostname (default: {BROKER})")
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--topic", default=TOPIC)
    parser.add_argument("--fast", action="store_true",
                        help="1 s between publishes")
    parser.add_argument("--instant", action="store_true",
                        help="No delay between publishes")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print payloads without connecting to MQTT")
    parser.add_argument("--seed", type=int, default=None,
                        help="Random seed for reproducibility")
    parser.add_argument("--continuous", action="store_true",
                        help="Loop forever, restarting each day (Ctrl-C to stop)")
    parser.add_argument("--days", type=int, default=1,
                        help="Number of days to simulate (default: 1; ignored if --continuous)")
    parser.add_argument("--csv", metavar="PATH", default=None,
                        help="Write all generated reports to a CSV file (e.g. data/sim.csv)")
    args = parser.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    # Determine inter-publish delay
    if args.instant:
        delay = 0.0
    elif args.fast:
        delay = 1.0
    else:
        delay = REPORT_EVERY.total_seconds()

    # Connect
    client = None
    if not args.dry_run:
        client = connect_mqtt(args.broker, args.port)
        time.sleep(1)  # let connection establish
        client.publish(TOPIC_LOG, json.dumps({"event": "sim_start"}))

    n_windows = int(timedelta(hours=24) / REPORT_EVERY)
    total_days = 0
    total_reports = 0
    day_num = 0
    day_start = SIM_START

    loop_forever = args.continuous
    max_days = args.days
    csv_rows: list[dict] = []

    print(f"Mode: {'continuous (Ctrl-C to stop)' if loop_forever else f'{max_days} day(s)'}")
    print(f"Delay: {delay}s between publishes\n")

    try:
        while loop_forever or day_num < max_days:
            day_num += 1
            print(f"\n{'=' * 52}")
            print(f"Day {day_num} — {day_start.strftime('%Y-%m-%d')}")
            print(f"{'Seq':>4}  {'Time':>8}  {'Intensity':>9}  "
                  f"{'US det':>6}  {'US sec':>7}  {'PIR':>4}")
            print("-" * 52)

            for i in range(n_windows):
                seq = total_reports + i + 1
                ts = day_start + REPORT_EVERY * (i + 1)
                hour_frac = ts.hour + ts.minute / 60.0
                intensity = traffic_intensity(hour_frac)

                report = generate_report(seq, ts, intensity)
                payload = json.dumps(report)

                us = report["ultrasonic"]
                print(f"{seq:4d}  {report['gps']['time']:>8}  {intensity:9.3f}  "
                      f"{us['detections']:6d}  {us['total_presence_sec']:7.1f}  "
                      f"{report['pir']['triggers']:4d}")

                if args.csv:
                    csv_rows.append({
                        "timestamp":      ts.isoformat(),
                        "seq":            report["seq"],
                        "gps_lat":        report["gps"]["lat"],
                        "gps_lon":        report["gps"]["lon"],
                        "us_detections":  report["ultrasonic"]["detections"],
                        "us_presence_sec": report["ultrasonic"]["total_presence_sec"],
                        "pir_triggers":   report["pir"]["triggers"],
                        "uptime_sec":     report["uptime_sec"],
                        "intensity":      round(intensity, 4),
                    })

                if client:
                    result = client.publish(args.topic, payload, qos=0)
                    if result.rc != 0:
                        print(f"  ⚠  Publish failed (rc={result.rc})")

                if delay > 0 and not (i == n_windows - 1 and not loop_forever and day_num == max_days):
                    time.sleep(delay)

            total_reports += n_windows
            total_days += 1
            day_start += timedelta(hours=24)
            print(f"\nDay {day_num} complete — {n_windows} reports published.")

    except KeyboardInterrupt:
        print(f"\n\nInterrupted after {total_days} day(s), {total_reports} reports.")

    # Done
    print(f"\n{'=' * 52}")
    print(f"Simulation complete — {total_reports} reports over {total_days} day(s).")
    if client:
        client.publish(TOPIC_LOG, json.dumps({"event": "sim_end",
                                               "days": total_days,
                                               "reports": total_reports}))
        time.sleep(0.5)
        client.loop_stop()
        client.disconnect()

    if args.csv and csv_rows:
        out = args.csv
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        with open(out, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=csv_rows[0].keys())
            writer.writeheader()
            writer.writerows(csv_rows)
        print(f"CSV written → {out}  ({len(csv_rows)} rows)")


if __name__ == "__main__":
    main()
