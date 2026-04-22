# Firmware

Arduino sketch for the ESP32 sensor node. Reads an HC-SR04 ultrasonic sensor, a PIR motion sensor, and a GPS module, then publishes aggregated reports over MQTT every 10 seconds.

The circuit is defined in `diagram.json` and can be simulated in [Wokwi](https://wokwi.com). A custom Wokwi chip in `gps_chip/` emulates a UART GPS module outputting NMEA GGA sentences for a fixed position near Burlington, MA.

## Hardware

| Component | Pin(s) |
|-----------|--------|
| HC-SR04 Trigger | D5 |
| HC-SR04 Echo | D18 |
| PIR sensor | D4 |
| GPS TX → ESP32 RX2 | GPIO 16 |
| GPS RX ← ESP32 TX2 | GPIO 17 |

Detection threshold: objects within **100 cm** of the ultrasonic sensor are counted.

## Dependencies

- [PubSubClient](https://github.com/knolleary/pubsubclient) — MQTT client (see `libraries.txt`)

## MQTT

| Setting | Value |
|---------|-------|
| Broker | `broker.hivemq.com` (Wokwi) / local Mosquitto (hardware) |
| Port | 1883 |
| Report topic | `eece5155/trektrak/report` |
| Log topic | `eece5155/trektrak/log` |

### Payload (published every 10 s)

```json
{
  "seq": 42,
  "gps": {
    "lat": "42.504800",
    "lon": "-71.195600",
    "time": "14:30:00 UTC"
  },
  "ultrasonic": {
    "detections": 3,
    "total_presence_sec": 7.4
  },
  "pir": {
    "triggers": 5
  },
  "uptime_sec": 420
}
```

Counters reset after each report. `total_presence_sec` accumulates while an object is within range; if an object is still present at report time, the ongoing duration is included.

## Running in Wokwi

1. Open [wokwi.com](https://wokwi.com) and import `diagram.json`.
2. Add the `gps_chip/` directory as a custom chip.
3. Press Play — the sketch connects via the Wokwi-GUEST WiFi network and publishes to HiveMQ.
