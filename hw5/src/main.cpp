#include <Arduino.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ctime>

#include "secrets.h"

namespace {
constexpr uint8_t DHT_PIN = 15;
constexpr unsigned long PUBLISH_INTERVAL_MS = 30000;
constexpr unsigned long WIFI_RETRY_INITIAL_MS = 2000;
constexpr unsigned long WIFI_RETRY_MAX_MS = 30000;
constexpr unsigned long MQTT_RETRY_INITIAL_MS = 2000;
constexpr unsigned long MQTT_RETRY_MAX_MS = 30000;
constexpr unsigned long LOOP_DELAY_MS = 10;
constexpr unsigned long NETWORK_TIMEOUT_SECONDS = 10;
constexpr std::time_t MIN_VALID_EPOCH = 1700000000;
constexpr uint16_t MQTT_PORT = 8883;

constexpr char MQTT_CLIENT_ID[] = "esp32-volodya";
constexpr char MQTT_TOPIC[] =
    "iot-course/volodya/sensors/data";

DHT dht(DHT_PIN, DHT22);
WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);
unsigned long lastPublishAt = 0;
unsigned long lastWifiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;
unsigned long wifiRetryIntervalMs = WIFI_RETRY_INITIAL_MS;
unsigned long mqttRetryIntervalMs = MQTT_RETRY_INITIAL_MS;
bool wifiConnectAttempted = false;
bool mqttConnectAttempted = false;
bool clockSyncStarted = false;
bool clockSynchronized = false;

unsigned long nextRetryInterval(unsigned long current, unsigned long maximum) {
  return current >= maximum / 2 ? maximum : current * 2;
}

void connectWifi(unsigned long now) {
  if (WiFiClass::status() == WL_CONNECTED) {
    wifiRetryIntervalMs = WIFI_RETRY_INITIAL_MS;
    return;
  }

  if (wifiConnectAttempted &&
      now - lastWifiAttemptAt < wifiRetryIntervalMs) {
    return;
  }

  wifiConnectAttempted = true;
  lastWifiAttemptAt = now;
  Serial.printf("Connecting to WiFi %s...\n\r", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiRetryIntervalMs =
      nextRetryInterval(wifiRetryIntervalMs, WIFI_RETRY_MAX_MS);
}

bool syncClock() {
  if (std::time(nullptr) >= MIN_VALID_EPOCH) {
    if (!clockSynchronized) {
      clockSynchronized = true;
      Serial.println("Clock synchronized");
    }
    return true;
  }

  if (!clockSyncStarted && WiFiClass::status() == WL_CONNECTED) {
    clockSyncStarted = true;
    Serial.println("Synchronizing clock...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  }

  return false;
}

void connectMqtt(unsigned long now) {
  if (mqttClient.connected() || WiFiClass::status() != WL_CONNECTED ||
      !syncClock()) {
    return;
  }

  if (mqttConnectAttempted &&
      now - lastMqttAttemptAt < mqttRetryIntervalMs) {
    return;
  }

  mqttConnectAttempted = true;
  lastMqttAttemptAt = now;
  Serial.print("Connecting to AWS IoT...");

  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    mqttRetryIntervalMs = MQTT_RETRY_INITIAL_MS;
    Serial.println(" connected");
    return;
  }

  mqttRetryIntervalMs =
      nextRetryInterval(mqttRetryIntervalMs, MQTT_RETRY_MAX_MS);
  Serial.printf(" failed, state=%d; retrying later\n\r", mqttClient.state());
}

void publishTemperature() {
  const float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    Serial.println("DHT22 read failed");
    return;
  }

  char payload[48];
  snprintf(payload, sizeof(payload), "{\"temperature\":%.1f}", temperature);

  if (mqttClient.publish(MQTT_TOPIC, payload)) {
    Serial.printf("Published %s -> %s\n\r", MQTT_TOPIC, payload);
  } else {
    Serial.println("MQTT publish failed");
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println("start");
  dht.begin();

  WiFiClass::mode(WIFI_STA);

  tlsClient.setCACert(AWS_ROOT_CA);
  tlsClient.setCertificate(AWS_DEVICE_CERT);
  tlsClient.setPrivateKey(AWS_PRIVATE_KEY);
  tlsClient.setHandshakeTimeout(NETWORK_TIMEOUT_SECONDS);
  tlsClient.setTimeout(NETWORK_TIMEOUT_SECONDS);

  mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
  mqttClient.setSocketTimeout(NETWORK_TIMEOUT_SECONDS);
  connectWifi(millis());

  lastPublishAt = millis() - PUBLISH_INTERVAL_MS;
}

void loop() {
  const unsigned long now = millis();
  connectWifi(now);
  connectMqtt(now);

  if (mqttClient.connected()) {
    mqttClient.loop();

    if (now - lastPublishAt >= PUBLISH_INTERVAL_MS) {
      lastPublishAt = now;
      publishTemperature();
    }
  }

  delay(LOOP_DELAY_MS);
}