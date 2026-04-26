# Simulation

Generates and publishes 24 hours of synthetic sensor data (96 reports at 15-minute intervals) to an MQTT broker. Useful for testing the full pipeline without physical hardware.

## Traffic Model

Activity is modeled as three overlapping Gaussian peaks on top of a small overnight baseline:

| Period | Peak |
|--------|------|
| Morning rush | ~08:30 |
| Lunch | ~12:15 |
| Evening rush | ~17:30 |

Ultrasonic detections, dwell time, and PIR triggers are all derived from the intensity value at each 15-minute window, with Gaussian noise applied.

## Setup

```bash
pip install -r requirements.txt   # from repo root
# or just: pip install paho-mqtt
```

## Usage

```bash
# Real-time: 15-minute delay between publishes (mirrors actual sensor cadence)
python simulation.py

# Fast: 1-second delay — good for watching data flow through the pipeline
python simulation.py --fast

# Instant: no delay, publish all 96 reports immediately
python simulation.py --instant

# Dry-run: print payloads without connecting to MQTT
python simulation.py --dry-run

# Export all generated reports to a CSV for offline analysis
python simulation.py --fast --csv ../data/sim.csv

# Simulate multiple days
python simulation.py --fast --days 7 --csv ../data/week.csv

# Loop forever (Ctrl-C to stop)
python simulation.py --continuous

# Reproducible run
python simulation.py --fast --seed 42
```

Default broker is `localhost:1883` (the Mosquitto container). The script publishes to the `trektrak/sensor/report` topic.

## CSV Output Format

When `--csv PATH` is provided the script writes one row per 15-minute window:

| Column | Description |
|---|---|
| `timestamp` | ISO-8601 UTC timestamp (end of window) |
| `seq` | Report sequence number |
| `gps_lat` / `gps_lon` | Sensor GPS coordinates |
| `us_detections` | Ultrasonic detection count |
| `us_presence_sec` | Total detected presence (seconds) |
| `pir_triggers` | PIR motion trigger count |
| `uptime_sec` | Sensor uptime |
| `intensity` | Ground-truth traffic intensity 0–1 (from model) |
