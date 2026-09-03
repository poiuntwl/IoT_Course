#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

constexpr uint8_t LED_PIN = 19;

constexpr char MQTT_HOST[] = "test.mosquitto.org";
constexpr uint16_t MQTT_PORT = 1883;

constexpr char SENSOR_TOPIC[] =
        "iot-course/volodymyr_1de632fd-de4a-4802-99e0-11c756d002c4/sensors";

constexpr char COMMANDS_TOPIC[] =
        "iot-course/volodymyr_1de632fd-de4a-4802-99e0-11c756d002c4/commands";

constexpr char SENSOR_PAYLOAD_FORMAT[] =
        R"({"temperature":%s,"humidity":%s})";

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static void ensureWifi();

static void ensureMQTT();

void setup() {
    Serial.begin(115200);

    WiFi.begin("Wokwi-GUEST");

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback([](char *topic, const byte *payload, const unsigned int length) {
        String message;

        for (unsigned int i = 0; i < length; ++i) {
            message += static_cast<char>(payload[i]);
        }

        Serial.printf("Topic: %s\r\n", topic);
        Serial.printf("Payload: %s\r\n", message.c_str());
    });

    if (mqttClient.connect("48166572-b6cc-4618-b5fd-fb0ea59f595d")) {
        mqttClient.subscribe(SENSOR_TOPIC);
        mqttClient.subscribe(COMMANDS_TOPIC);
    }
}


void loop() {
    ensureWifi();
    ensureMQTT();
}


static void ensureWifi() {
    static ulong lastWifiCheckTs;

    if (WiFi.isConnected()) {
        return;
    }
    if (millis() - lastWifiCheckTs >= 5000) {
        lastWifiCheckTs = millis();
        WiFi.begin();
    }
}

static void ensureMQTT() {
    static ulong lastMqttCheckTs;

    if (WiFi.isConnected() == false) {
        return;
    }

    if (mqttClient.connected()) {
        mqttClient.loop();
        return;
    }

    if (millis() - lastMqttCheckTs >= 5000) {
        lastMqttCheckTs = millis();
        if (mqttClient.connect("48166572-b6cc-4618-b5fd-fb0ea59f595d")) {
            mqttClient.subscribe(SENSOR_TOPIC);
            mqttClient.subscribe(COMMANDS_TOPIC);
        }
    }
}
