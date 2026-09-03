#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

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
    mqttClient.connect("993518a0-20c4-41ad-8228-a73bb2e601f2");
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
        mqttClient.connect("993518a0-20c4-41ad-8228-a73bb2e601f2");
    }
}
