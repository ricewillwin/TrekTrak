
# IoT Pipeline

Docker Compose stack that ingests, stores, and visualizes sensor data from TrekTrak nodes.

## Setup

1. Copy `env-temp` to `.env` and fill in values:

```bash
cp env-temp .env
# edit .env: set INFLUXDB_PASSWORD, INFLUXDB_TOKEN, GRAFANA_PASSWORD, DOMAIN
```

2. Start all services:

```bash
docker compose up -d
```

## Services

The following services are defined in `docker-compose.yml`. With Traefik running, each service is also reachable at `<service>.${DOMAIN}` on your local network.

### Traefik

Reverse proxy that routes HTTP traffic to the other containers by hostname.

- Dashboard: `http://localhost:8080` or `http://traefik.${DOMAIN}`

```yaml
traefik:
    image: traefik:v3.6
    ports:
        - "80:80"
        - "8080:8080"  # dashboard
```

### Mosquitto

MQTT broker. Sensor nodes and `simulation.py` publish to it; Node-RED subscribes.

- Port: `1883`
- Config: `config/mosquitto/mosquitto.conf`

```yaml
mosquitto:
    image: eclipse-mosquitto:2
    ports:
        - "1883:1883"
```

### InfluxDB

Time-series database that stores all sensor measurements.

- UI: `http://localhost:8086` or `http://influxdb.${DOMAIN}`
- Organization: `${INFLUXDB_ORG}` (default: `iot`)
- Bucket: `${INFLUXDB_BUCKET}` (default: `sensors`)

```yaml
influxdb:
    image: influxdb:2
    environment:
        DOCKER_INFLUXDB_INIT_MODE: setup
        DOCKER_INFLUXDB_INIT_USERNAME: admin
        DOCKER_INFLUXDB_INIT_PASSWORD: ${INFLUXDB_PASSWORD}
        DOCKER_INFLUXDB_INIT_ORG: ${INFLUXDB_ORG:-iot}
        DOCKER_INFLUXDB_INIT_BUCKET: ${INFLUXDB_BUCKET:-sensors}
        DOCKER_INFLUXDB_INIT_ADMIN_TOKEN: ${INFLUXDB_TOKEN}
```

### Node-RED

Visual flow editor that subscribes to MQTT, parses payloads, and writes records to InfluxDB.

- UI: `http://localhost:1880` or `http://nodered.${DOMAIN}`
- Import the flow from `../analysis/nodered.json` after first launch.

### Grafana

Dashboard for visualizing data stored in InfluxDB.

- UI: `http://localhost:3000` or `http://grafana.${DOMAIN}`
- Default login: `admin` / `${GRAFANA_PASSWORD}`
- Add InfluxDB as a data source pointing to `http://influxdb:8086`.

## Environment Variables

| Variable | Description |
|----------|-------------|
| `DOMAIN` | Local domain used by Traefik (e.g. `iot.home.local`) |
| `INFLUXDB_PASSWORD` | InfluxDB admin password |
| `INFLUXDB_TOKEN` | InfluxDB API token |
| `INFLUXDB_ORG` | InfluxDB organization name (default: `iot`) |
| `INFLUXDB_BUCKET` | InfluxDB bucket name (default: `sensors`) |
| `GRAFANA_PASSWORD` | Grafana admin password |
| `TZ` | Timezone for containers (e.g. `America/New_York`) |
