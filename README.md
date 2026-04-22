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
| [analysis/](analysis/) | Data analysis: Node-RED flows, Jupyter notebooks, and other exploration tools |
| [data/](data/) | Exported datasets and analysis outputs |
| [submissions/](submissions/) | Course deliverable snapshots (E1, E2, E3) |

## Quick Start

1. Copy `pipeline/env-temp` to `pipeline/.env` and fill in your credentials.
2. Start the pipeline: `cd pipeline && docker compose up -d`
3. Import `analysis/nodered.json` into Node-RED (http://localhost:1880).
4. Flash the firmware to an ESP32, or open `firmware/diagram.json` in [Wokwi](https://wokwi.com).
5. To test without hardware: `cd simulation && python simulation.py --fast`

