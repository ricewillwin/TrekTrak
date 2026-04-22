# Analysis

Data analysis and exploration for TrekTrak sensor data. This directory will grow to include Node-RED flows, Jupyter notebooks, and any other tools used to process and explore the collected data.

## Contents

| File | Description |
|------|-------------|
| `nodered.json` | Node-RED flow that subscribes to `trektrak/sensor/#`, parses JSON payloads, and routes data to InfluxDB |

## Importing the Node-RED Flow

1. Open Node-RED at `http://localhost:1880`.
2. Click the hamburger menu → **Import**.
3. Paste the contents of `nodered.json` (or use **select a file to import**).
4. Deploy.

The flow expects Mosquitto reachable at `mosquitto:1883` and InfluxDB at `influxdb:8086` — both are provided by the Docker Compose stack in `../pipeline/`.
