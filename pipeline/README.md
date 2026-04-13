
# IoT Pipeline

## Services

The following services are defined in the docker-compose.md file

### Traefik

Traefik is a reverse proxy which can be used to give domains to the various services which make up the IoT stack.

```json
traefik:
    image: traefik:v3.6
    container_name: traefik
    restart: unless-stopped
    command:
        - --api.dashboard=true
        - --api.insecure=true
        - --providers.docker=true
        - --providers.docker.exposedbydefault=false
        - --providers.docker.network=iot
        - --entrypoints.web.address=:80
    ports:
        - "80:80"
        - "8080:8080"  # dashboard
    volumes:
        - /var/run/docker.sock:/var/run/docker.sock:ro
    networks:
        - iot
    labels:
        - "traefik.enable=true"
        - "traefik.http.routers.traefik.rule=Host(`traefik.${DOMAIN}`)"
        - "traefik.http.routers.traefik.entrypoints=web"
        - "traefik.http.routers.traefik.service=api@internal"
```

### Mosquitto

```json
mosquitto:
    image: eclipse-mosquitto:2
    container_name: mosquitto
    restart: unless-stopped
    ports:
        - "1883:1883"
    volumes:
        - ./config/mosquitto/mosquitto.conf:/mosquitto/config/mosquitto.conf:ro
        - mosquitto_data:/mosquitto/data
    networks:
        - iot
```

### InfluxDB

```json
influxdb:
    image: influxdb:2
    container_name: influxdb
    restart: unless-stopped
    environment:
        DOCKER_INFLUXDB_INIT_MODE: setup
        DOCKER_INFLUXDB_INIT_USERNAME: admin
        DOCKER_INFLUXDB_INIT_PASSWORD: ${INFLUXDB_PASSWORD}
        DOCKER_INFLUXDB_INIT_ORG: ${INFLUXDB_ORG:-iot}
        DOCKER_INFLUXDB_INIT_BUCKET: ${INFLUXDB_BUCKET:-sensors}
        DOCKER_INFLUXDB_INIT_ADMIN_TOKEN: ${INFLUXDB_TOKEN}
    volumes:
        - influxdb_data:/var/lib/influxdb2
    networks:
        - iot
    labels:
        - "traefik.enable=true"
        - "traefik.http.routers.influxdb.rule=Host(`influxdb.${DOMAIN}`)"
        - "traefik.http.routers.influxdb.entrypoints=web"
        - "traefik.http.services.influxdb.loadbalancer.server.port=8086"

```

### Node-Red


### Grafana


### Analysis
