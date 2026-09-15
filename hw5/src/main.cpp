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
constexpr uint16_t MQTT_PORT = 8883;

constexpr char MQTT_CLIENT_ID[] = "volodya-3cf39f28-7a18-474c-bd57-f50d7be3f37a";
constexpr char MQTT_TOPIC[] =
    "iot-course/volodya-3cf39f28-7a18-474c-bd57-f50d7be3f37a/sensors/data";

DHT dht(DHT_PIN, DHT22);
WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);
unsigned long lastPublishAt = 0;

void connectWifi() {
  if (WiFiClass::status() == WL_CONNECTED) {
    return;
  }

  Serial.printf("Connecting to WiFi %s", WIFI_SSID);
  WiFiClass::mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFiClass::status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println(" connected");
}

void syncClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Synchronizing clock");

  std::time_t now = std::time(nullptr);
  while (now < 1700000000) {
    delay(500);
    Serial.print('.');
    now = std::time(nullptr);
  }

  Serial.println(" synchronized");
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to AWS IoT...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println(" connected");
      return;
    }

    Serial.printf(" failed, state=%d; retrying\n\r", mqttClient.state());
    delay(2000);
  }
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

  connectWifi();
  syncClock();

  tlsClient.setCACert(AWS_ROOT_CA);
  tlsClient.setCertificate(AWS_DEVICE_CERT);
  tlsClient.setPrivateKey(AWS_PRIVATE_KEY);

  mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
  connectMqtt();

  lastPublishAt = millis() - PUBLISH_INTERVAL_MS;
}

void loop() {
  connectWifi();
  connectMqtt();
  mqttClient.loop();

  const unsigned long now = millis();
  if (now - lastPublishAt >= PUBLISH_INTERVAL_MS) {
    lastPublishAt = now;
    publishTemperature();
  }
}