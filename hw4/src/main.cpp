#include <Arduino.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>

constexpr int DHT_PIN = 26;
constexpr int BTN_PIN = 19;
constexpr char MQTT_HOST[] = "test.mosquitto.org";
constexpr uint16_t MQTT_PORT = 1883;

static DHTesp dht;

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

void setup() {
    Serial.begin(115200);
    dht.setup(DHT_PIN, DHTesp::DHT22);
    pinMode(BTN_PIN, INPUT_PULLUP);

    WiFi.begin("Wokwi-GUEST");
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}

static void pubTnH();

static void ensureWifi();

static void ensureMQTT();

static ulong lastSentTs;
static ulong lastWifiCheckTs;
static ulong lastMqttCheckTs;

void loop() {
    ensureWifi();
    ensureMQTT();
    pubTnH();
}


static void pubTnH() {
    if (millis() - lastSentTs >= 10000) {
        Serial.println("publish TH");
    }
}

static void ensureWifi() {
    if (WiFi.isConnected()) {
        return;
    }
    if (millis() - lastWifiCheckTs >= 5000) {
        lastWifiCheckTs = millis();
        WiFi.begin();
    }
}


static void ensureMQTT() {
    if (WiFi.isConnected() == false) {
        return;
    }

    if (mqttClient.connected()) {
        mqttClient.loop();
        return;
    }

    if (millis() - lastMqttCheckTs >= 5000) {
        lastMqttCheckTs = millis();
        mqttClient.connect("993518a0-20c4-41ad-8228-a73bb2e601f2");
    }
}
