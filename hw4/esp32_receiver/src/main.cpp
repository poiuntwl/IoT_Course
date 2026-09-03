#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

constexpr uint8_t LED_PIN = 19;

static constexpr char MQTT_HOST[] = "test.mosquitto.org";
static constexpr uint16_t MQTT_PORT = 1883;
static constexpr uint8_t BLINK_COUNT = 3;
static constexpr uint8_t RETRY_COUNT = 3;
static constexpr ulong BLINK_INTERVAL_MS = 500;

constexpr char SENSOR_TEMP_TOPIC[] =
        "iot-course/volodymyr_1de632fd-de4a-4802-99e0-11c756d002c4/sensors/temperature";
constexpr char SENSOR_HUM_TOPIC[] =
        "iot-course/volodymyr_1de632fd-de4a-4802-99e0-11c756d002c4/sensors/humidity";

constexpr char COMMANDS_TOPIC[] =
        "iot-course/volodymyr_1de632fd-de4a-4802-99e0-11c756d002c4/commands";

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static bool blinking = false;
static bool blinkState = false;
static uint8_t blinkTransitions = 0;
static ulong lastBlinkTs = 0;

static uint8_t currentConnectRetryWifi;
static uint8_t currentConnectRetryMQTT;

static void ensureWifi();

static void ensureMQTT();

static void execCommand(const byte *payload, unsigned int length);

static void updateSensors(byte *payload, unsigned int length, const char *topic);

static void updateBlink();

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    WiFi.begin("Wokwi-GUEST");

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback([](
    const char *topic,
    byte *payload,
    const unsigned int length
) {
            if (strcmp(topic, COMMANDS_TOPIC) == 0) {
                execCommand(payload, length);
            }

            if (strcmp(topic, SENSOR_TEMP_TOPIC) == 0
                || strcmp(topic, SENSOR_HUM_TOPIC) == 0) {
                updateSensors(payload, length, topic);
            }

            JsonDocument doc;
        });

    if (mqttClient.connect("48166572-b6cc-4618-b5fd-fb0ea59f595d")) {
        mqttClient.subscribe(SENSOR_TEMP_TOPIC);
        mqttClient.subscribe(COMMANDS_TOPIC);
    }
}

void loop() {
    ensureWifi();
    ensureMQTT();

    updateBlink();
}


static void ensureWifi() {
    static ulong lastWifiCheckTs;

    if (WiFi.isConnected()) {
        currentConnectRetryWifi = 0;
        return;
    }

    if (currentConnectRetryWifi >= RETRY_COUNT) {
        return;
    }

    if (millis() - lastWifiCheckTs >= 5000) {
        lastWifiCheckTs = millis();
        WiFi.begin();
        currentConnectRetryWifi++;
    }
}

static void ensureMQTT() {
    static ulong lastMqttCheckTs;

    if (WiFi.isConnected() == false) {
        return;
    }

    if (mqttClient.connected()) {
        mqttClient.loop();
        currentConnectRetryMQTT = 0;
        return;
    }

    if (currentConnectRetryMQTT >= RETRY_COUNT) {
        return;
    }

    if (millis() - lastMqttCheckTs >= 5000) {
        lastMqttCheckTs = millis();
        currentConnectRetryMQTT++;
        if (mqttClient.connect("48166572-b6cc-4618-b5fd-fb0ea59f595d")) {
            currentConnectRetryMQTT = 0;
            mqttClient.subscribe(SENSOR_TEMP_TOPIC);
            mqttClient.subscribe(COMMANDS_TOPIC);
        }
    }
}

void execCommand(const byte *payload, const unsigned int length) {
    constexpr size_t MAX_COMMAND_LENGTH = 64;

    if (length >= MAX_COMMAND_LENGTH) {
        Serial.println("Command too long");
        return;
    }

    char message[MAX_COMMAND_LENGTH];

    memcpy(message, payload, length);
    message[length] = '\0';

    if (strcmp(message, "manual_read") == 0) {
        Serial.println("Manual trigger received");
        blinking = true;
        blinkState = true;
        blinkTransitions = 0;
        lastBlinkTs = millis();

        digitalWrite(LED_PIN, HIGH);
    }
}

void updateSensors(byte *payload, unsigned int length, const char *topic) {
    JsonDocument doc;

    const DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
        Serial.printf("JSON error: %s\n", error.c_str());
        return;
    }

    if (strcmp(topic, SENSOR_TEMP_TOPIC) == 0) {
        const float temperature = doc["temperature"];
        if (temperature > 26) {
            digitalWrite(LED_PIN, HIGH);
        } else if (temperature < 20) {
            digitalWrite(LED_PIN, LOW);
        }
    }

    if (strcmp(topic, SENSOR_HUM_TOPIC) == 0) {
        const float humidity = doc["humidity"];
    }
}

void updateBlink() {
    if (!blinking) {
        return;
    }

    if (millis() - lastBlinkTs < BLINK_INTERVAL_MS) {
        return;
    }

    lastBlinkTs = millis();

    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState ? HIGH : LOW);

    ++blinkTransitions;

    if (blinkTransitions >= BLINK_COUNT * 2) {
        blinking = false;
        blinkState = false;
        digitalWrite(LED_PIN, LOW);
    }
}
