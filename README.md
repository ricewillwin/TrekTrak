# TrekTrak

A bike path monitoring IoT system built for EECE 5155 at Northeastern University. Distributed sensor nodes detect and count pedestrians and cyclists passing a fixed point on a trail, and forward that data to a central pipeline for storage and visualization.

## System Architecture

```
[ESP32 Sensor Node]
  HC-SR04 Ultrasonic + PIR + GPS
        |
     WiFi / MQTT
        |
  [Mosquitto Broker]   <── simulation.py (for testing)
        |
    [Node-RED]
        |
    [InfluxDB]
        |
    [Grafana]
```

The gateway runs on a Raspberry Pi CM4. All pipeline services are containerized via Docker Compose and fronted by a Traefik reverse proxy.

## Repository Layout

| Directory | Purpose |
|-----------|---------|
| [firmware/](firmware/) | ESP32 Arduino sketch + Wokwi GPS chip |
| [pipeline/](pipeline/) | Docker Compose stack (Traefik, Mosquitto, InfluxDB, Node-RED, Grafana) |
| [simulation/](simulation/) | Python script to generate and publish 24 h of synthetic sensor data |
| [analysis/](analysis/) | Node-RED flow, Jupyter notebooks, and ML classifier |
| [data/](data/) | Exported datasets (`sim.csv`) |
| [submissions/](submissions/) | Course deliverable snapshots |

## Quick Start

```bash
# 1. Install Python dependencies
python -m venv iotvenv && source iotvenv/bin/activate
pip install -r requirements.txt

# 2. Start the pipeline
cd pipeline && docker compose up -d

# 3. Import analysis/nodered.json into Node-RED at http://localhost:1880
#    (hamburger → Import; update the InfluxDB node with your token and host IP)

# 4. Run the simulation — publishes 96 reports and writes a CSV
python simulation/simulation.py --fast --csv data/sim.csv

# 5. Open Grafana at http://localhost:3000 (credentials in pipeline/.env)

# 6. Explore the data
jupyter notebook analysis/initial_analysis.ipynb

# 7. Run the pedestrian/cyclist ML classifier
jupyter notebook analysis/ml_classifier.ipynb
```

To test with real hardware, flash `firmware/` to an ESP32 or open `firmware/diagram.json` in [Wokwi](https://wokwi.com).

