#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- Pin Definitions ---
#define GPS_RX    16   // ESP32 RX2 <- GPS TX
#define GPS_TX    17   // ESP32 TX2 -> GPS RX
#define TRIG_PIN   5
#define ECHO_PIN  18
#define PIR_PIN    4

// --- WiFi & MQTT ---
const char* WIFI_SSID       = "Wokwi-GUEST";
const char* WIFI_PASS       = "";
const char* MQTT_BROKER     = "broker.hivemq.com";
const int   MQTT_PORT       = 1883;
const char* MQTT_TOPIC      = "esp32/sensorhub/report";
const char* MQTT_TOPIC_LOG  = "esp32/sensorhub/log";

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// --- GPS State ---
String gpsLat  = "N/A";
String gpsLon  = "N/A";
String gpsTime = "N/A";
String nmeaBuf = "";

// --- Ultrasonic Detection State ---
const float DETECT_THRESHOLD_CM = 100.0;
const unsigned long DEBOUNCE_MS = 300;

int      usDetectionCount      = 0;
unsigned long usTotalDurationMs = 0;
bool     usObjectPresent       = false;
unsigned long usObjectStartMs  = 0;
unsigned long usLastReadMs     = 0;
const unsigned long US_READ_INTERVAL = 200; // read every 200ms

// --- PIR State ---
int  pirTriggerCount = 0;
bool pirPrevState    = false;

// --- Reporting ---
const unsigned long REPORT_INTERVAL_MS = 900000; // 15 minutes
unsigned long lastReportMs = 0;
unsigned long reportSeqNum = 0;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  Serial.println("\n=== ESP32 Sensor Hub ===");
  Serial.println("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setBufferSize(512);
  reconnectMQTT();

  lastReportMs = millis();
  Serial.println("System ready. Reporting every 15 minutes.\n");
}

// ============================================================
// MQTT
// ============================================================
void reconnectMQTT() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    String clientId = "ESP32Hub-" + String(random(0xFFFF), HEX);
    Serial.printf("MQTT connecting as %s ...", clientId.c_str());
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" OK!");
      mqtt.publish(MQTT_TOPIC_LOG, "{\"event\":\"online\"}");
      return;
    }
    Serial.printf(" failed (rc=%d), retrying...\n", mqtt.state());
    delay(2000);
    attempts++;
  }
}

// ============================================================
// GPS NMEA PARSER
// ============================================================
void readGPS() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      nmeaBuf.trim();
      if (nmeaBuf.startsWith("$GPGGA") || nmeaBuf.startsWith("$GNGGA")) {
        parseGGA(nmeaBuf);
      }
      nmeaBuf = "";
    } else if (c != '\r') {
      nmeaBuf += c;
    }
  }
}

void parseGGA(const String &sentence) {
  // $GPGGA,time,lat,N/S,lon,E/W,fix,sats,hdop,alt,M,...
  int idx[15];
  int cnt = 0;
  for (unsigned int i = 0; i < sentence.length() && cnt < 15; i++) {
    if (sentence.charAt(i) == ',') idx[cnt++] = i;
  }
  if (cnt < 6) return;

  String rawTime = sentence.substring(idx[0] + 1, idx[1]);
  String rawLat  = sentence.substring(idx[1] + 1, idx[2]);
  String latDir  = sentence.substring(idx[2] + 1, idx[3]);
  String rawLon  = sentence.substring(idx[3] + 1, idx[4]);
  String lonDir  = sentence.substring(idx[4] + 1, idx[5]);

  if (rawLat.length() > 0 && rawLon.length() > 0) {
    // Convert NMEA ddmm.mmmm to decimal degrees
    gpsLat = nmeaToDecimal(rawLat, latDir);
    gpsLon = nmeaToDecimal(rawLon, lonDir);
    if (rawTime.length() >= 6) {
      gpsTime = rawTime.substring(0, 2) + ":" +
                rawTime.substring(2, 4) + ":" +
                rawTime.substring(4, 6) + " UTC";
    }
  }
}

String nmeaToDecimal(const String &raw, const String &dir) {
  // raw = "ddmm.mmmm" or "dddmm.mmmm"
  int dotPos = raw.indexOf('.');
  if (dotPos < 2) return raw;

  int degLen = (dotPos > 4) ? 3 : 2; // lon has 3-digit degrees
  float deg = raw.substring(0, degLen).toFloat();
  float min = raw.substring(degLen).toFloat();
  float decimal = deg + (min / 60.0);

  if (dir == "S" || dir == "W") decimal = -decimal;

  return String(decimal, 6);
}

// ============================================================
// ULTRASONIC SENSOR
// ============================================================
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long us = pulseIn(ECHO_PIN, HIGH, 30000);
  if (us == 0) return -1.0;
  return us * 0.034 / 2.0;
}

void processUltrasonic() {
  if (millis() - usLastReadMs < US_READ_INTERVAL) return;
  usLastReadMs = millis();

  float dist = readDistanceCm();

  bool detected = (dist > 0 && dist < DETECT_THRESHOLD_CM);

  if (detected && !usObjectPresent) {
    // New detection
    usObjectPresent = true;
    usObjectStartMs = millis();
    usDetectionCount++;
    Serial.printf("[US] Object detected at %.1f cm  (total count: %d)\n",
                  dist, usDetectionCount);
  } else if (!detected && usObjectPresent) {
    // Object left
    unsigned long dur = millis() - usObjectStartMs;
    usTotalDurationMs += dur;
    usObjectPresent = false;
    Serial.printf("[US] Object departed after %lu ms\n", dur);
  }
}

// ============================================================
// PIR SENSOR
// ============================================================
void processPIR() {
  bool state = digitalRead(PIR_PIN);
  if (state && !pirPrevState) {
    pirTriggerCount++;
    Serial.printf("[PIR] Motion detected! (total count: %d)\n", pirTriggerCount);
  }
  pirPrevState = state;
}

// ============================================================
// REPORT
// ============================================================
void sendReport() {
  // Include ongoing detection duration if object still present
  unsigned long totalDur = usTotalDurationMs;
  if (usObjectPresent) {
    totalDur += (millis() - usObjectStartMs);
  }

  reportSeqNum++;

  // Build JSON
  char payload[400];
  snprintf(payload, sizeof(payload),
    "{"
      "\"seq\":%lu,"
      "\"gps\":{\"lat\":%s,\"lon\":%s,\"time\":\"%s\"},"
      "\"ultrasonic\":{\"detections\":%d,\"total_presence_sec\":%.1f},"
      "\"pir\":{\"triggers\":%d},"
      "\"uptime_sec\":%lu"
    "}",
    reportSeqNum,
    gpsLat.c_str(), gpsLon.c_str(), gpsTime.c_str(),
    usDetectionCount, totalDur / 1000.0,
    pirTriggerCount,
    millis() / 1000
  );

  Serial.println("\n--- MQTT REPORT ---");
  Serial.println(payload);

  if (mqtt.publish(MQTT_TOPIC, payload)) {
    Serial.println("Report published OK\n");
  } else {
    Serial.println("Publish FAILED\n");
  }

  // Reset counters for next window
  usDetectionCount    = 0;
  usTotalDurationMs   = 0;
  pirTriggerCount     = 0;
  if (usObjectPresent) {
    usObjectStartMs = millis(); // reset ongoing measurement
  }
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  if (!mqtt.connected()) reconnectMQTT();
  mqtt.loop();

  readGPS();
  processUltrasonic();
  processPIR();

  if (millis() - lastReportMs >= REPORT_INTERVAL_MS) {
    sendReport();
    lastReportMs = millis();
  }

  delay(50);
}
