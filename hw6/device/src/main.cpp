#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ctime>

#include "secrets.h"

namespace {
constexpr uint8_t LED_PIN = 2;
constexpr uint8_t SENSOR_PIN = 15;
constexpr uint8_t SENSOR_TYPE = DHT22;
constexpr uint16_t MQTT_PORT = 8883;
constexpr unsigned long WIFI_RETRY_INITIAL_MS = 2000;
constexpr unsigned long WIFI_RETRY_MAX_MS = 30000;
constexpr unsigned long MQTT_RETRY_INITIAL_MS = 2000;
constexpr unsigned long MQTT_RETRY_MAX_MS = 30000;
constexpr unsigned long CLOCK_SYNC_TIMEOUT_MS = 10000;
constexpr unsigned long CLOCK_RETRY_MS = 30000;
constexpr unsigned long SENSOR_PUBLISH_INTERVAL_MS = 5000;
constexpr unsigned long NETWORK_TIMEOUT_SECONDS = 10;
constexpr std::time_t MIN_VALID_EPOCH = 1700000000;

constexpr char MQTT_CLIENT_ID[] = "esp32-volodya";
constexpr char AWS_IOT_ENDPOINT[] =
    "a3uy4feb9m7i98-ats.iot.eu-central-1.amazonaws.com";
constexpr char LED_COMMAND_TOPIC[] = "iot-course/volodya/commands/led";
constexpr char SENSOR_TOPIC[] = "iot-course/volodya/sensors/data";
constexpr char EVENTS_TOPIC[] = "iot-course/volodya/events";

WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);
DHT dht(SENSOR_PIN, SENSOR_TYPE);

unsigned long lastWifiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;
unsigned long wifiRetryIntervalMs = WIFI_RETRY_INITIAL_MS;
unsigned long mqttRetryIntervalMs = MQTT_RETRY_INITIAL_MS;
bool wifiConnectAttempted = false;
bool mqttConnectAttempted = false;
bool wifiWasConnected = false;

bool clockSyncInProgress = false;
bool clockSynchronized = false;
unsigned long clockSyncStartedAt = 0;
unsigned long lastClockAttemptAt = 0;

bool ledCommandPending = false;
bool pendingLedOn = false;
bool ledOn = false;
bool eventPending = false;
bool eventLedOn = false;
unsigned long lastSensorPublishAt = 0;

unsigned long nextRetryInterval(unsigned long current, unsigned long maximum) {
  return current >= maximum / 2 ? maximum : current * 2;
}

void ensureWifi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      Serial.println("WiFi connected");
    }

    wifiRetryIntervalMs = WIFI_RETRY_INITIAL_MS;
    wifiConnectAttempted = false;
    return;
  }

  if (wifiWasConnected) {
    wifiWasConnected = false;
    Serial.println("WiFi connection lost");
  }

  if (wifiConnectAttempted &&
      now - lastWifiAttemptAt < wifiRetryIntervalMs) {
    return;
  }

  wifiConnectAttempted = true;
  lastWifiAttemptAt = now;
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  Serial.println("...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiRetryIntervalMs =
      nextRetryInterval(wifiRetryIntervalMs, WIFI_RETRY_MAX_MS);
}

bool ensureClock(unsigned long now) {
  if (std::time(nullptr) >= MIN_VALID_EPOCH) {
    if (!clockSynchronized) {
      clockSynchronized = true;
      clockSyncInProgress = false;
      Serial.println("Clock synchronized");
    }

    return true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  if (clockSyncInProgress) {
    if (now - clockSyncStartedAt < CLOCK_SYNC_TIMEOUT_MS) {
      return false;
    }

    clockSyncInProgress = false;
    Serial.println("Clock synchronization timed out");
  }

  if (lastClockAttemptAt != 0 &&
      now - lastClockAttemptAt < CLOCK_RETRY_MS) {
    return false;
  }

  lastClockAttemptAt = now;
  clockSyncStartedAt = now;
  clockSyncInProgress = true;
  Serial.println("Synchronizing clock...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  return false;
}

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic, LED_COMMAND_TOPIC) != 0) {
    return;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("Invalid command JSON: ");
    Serial.println(error.c_str());
    return;
  }

  if (!doc["action"].is<const char *>() ||
      !doc["value"].is<const char *>()) {
    Serial.println("LED command must contain string action and value");
    return;
  }

  const char *action = doc["action"];
  const char *value = doc["value"];

  if (strcmp(action, "set") != 0) {
    Serial.println("Unsupported LED command action");
    return;
  }

  if (strcmp(value, "on") == 0) {
    pendingLedOn = true;
  } else if (strcmp(value, "off") == 0) {
    pendingLedOn = false;
  } else {
    Serial.println("Unsupported LED command value");
    return;
  }

  ledCommandPending = true;
}

void ensureMqtt(unsigned long now) {
  if (mqttClient.connected()) {
    mqttRetryIntervalMs = MQTT_RETRY_INITIAL_MS;
    mqttConnectAttempted = false;
    return;
  }

  if (WiFi.status() != WL_CONNECTED || !ensureClock(now)) {
    return;
  }

  if (mqttConnectAttempted &&
      now - lastMqttAttemptAt < mqttRetryIntervalMs) {
    return;
  }

  mqttConnectAttempted = true;
  lastMqttAttemptAt = now;
  Serial.print("Connecting to AWS IoT...");

  if (!mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.print(" failed, state=");
    Serial.println(mqttClient.state());
    mqttRetryIntervalMs =
        nextRetryInterval(mqttRetryIntervalMs, MQTT_RETRY_MAX_MS);
    return;
  }

  if (!mqttClient.subscribe(LED_COMMAND_TOPIC, 1)) {
    Serial.println(" connected, but command subscription failed");
    mqttClient.disconnect();
    mqttRetryIntervalMs =
        nextRetryInterval(mqttRetryIntervalMs, MQTT_RETRY_MAX_MS);
    return;
  }

  mqttRetryIntervalMs = MQTT_RETRY_INITIAL_MS;
  mqttConnectAttempted = false;
  Serial.println(" connected and subscribed");
}

void applyPendingLedCommand() {
  if (!ledCommandPending) {
    return;
  }

  ledCommandPending = false;
  if (ledOn == pendingLedOn) {
    Serial.print("LED already ");
    Serial.println(ledOn ? "on" : "off");
    return;
  }

  ledOn = pendingLedOn;
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);

  eventLedOn = ledOn;
  eventPending = true;

  Serial.print("LED ");
  Serial.println(ledOn ? "on" : "off");
}

void publishPendingEvent() {
  if (!eventPending || !mqttClient.connected()) {
    return;
  }

  char payload[96];
  snprintf(
      payload,
      sizeof(payload),
      "{\"event\":\"led_changed\",\"value\":\"%s\"}",
      eventLedOn ? "on" : "off"
  );

  if (!mqttClient.publish(EVENTS_TOPIC, payload)) {
    Serial.println("LED confirmation publish failed");
    return;
  }

  eventPending = false;
  Serial.print("Published ");
  Serial.print(EVENTS_TOPIC);
  Serial.print(" -> ");
  Serial.println(payload);
}

void publishSensorTelemetry(unsigned long now) {
  if (!mqttClient.connected() ||
      (lastSensorPublishAt != 0 &&
       now - lastSensorPublishAt < SENSOR_PUBLISH_INTERVAL_MS)) {
    return;
  }

  lastSensorPublishAt = now;

  const float temperature = dht.readTemperature();
  const float humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT22 sensor read failed");
    return;
  }

  const std::time_t measurementTime = std::time(nullptr);
  char payload[160];
  const int payloadLength = snprintf(
      payload,
      sizeof(payload),
      "{\"device_id\":\"%s\",\"temperature\":%.1f,\"humidity\":%.1f,"
      "\"timestamp\":%lld}",
      MQTT_CLIENT_ID,
      temperature,
      humidity,
      static_cast<long long>(measurementTime)
  );
  if (payloadLength < 0 || payloadLength >= static_cast<int>(sizeof(payload))) {
    Serial.println("Sensor payload is too large");
    return;
  }

  if (!mqttClient.publish(SENSOR_TOPIC, payload)) {
    Serial.println("Sensor telemetry publish failed");
    return;
  }

  Serial.print("Published ");
  Serial.print(SENSOR_TOPIC);
  Serial.print(" -> ");
  Serial.println(payload);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();

  WiFi.mode(WIFI_STA);

  tlsClient.setCACert(AWS_ROOT_CA);
  tlsClient.setCertificate(AWS_DEVICE_CERT);
  tlsClient.setPrivateKey(AWS_PRIVATE_KEY);
  tlsClient.setHandshakeTimeout(NETWORK_TIMEOUT_SECONDS);

  mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
  mqttClient.setSocketTimeout(NETWORK_TIMEOUT_SECONDS);
  mqttClient.setCallback(onMqttMessage);

  ensureWifi(millis());
}

void loop() {
  const unsigned long now = millis();

  ensureWifi(now);
  ensureMqtt(now);

  if (mqttClient.connected()) {
    mqttClient.loop();
    publishSensorTelemetry(now);
    applyPendingLedCommand();
    publishPendingEvent();
  }
}