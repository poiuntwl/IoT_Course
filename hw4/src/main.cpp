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

static void pubTnH();

static void ensureWifi();

static void ensureMQTT();

static void formatWithFallback(float val, char *buffer, size_t bufferSize);

static bool isBtnPressed();

static bool isBtnClicked();

void setup() {
    Serial.begin(115200);
    dht.setup(DHT_PIN, DHTesp::DHT22);
    pinMode(BTN_PIN, INPUT_PULLUP);

    WiFi.begin("Wokwi-GUEST");

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.connect("993518a0-20c4-41ad-8228-a73bb2e601f2");
}

void loop() {
    ensureWifi();
    ensureMQTT();
    pubTnH();

    const bool btnClicked = isBtnClicked();
    if (btnClicked) {
        Serial.println("clicked");
    }
}


static void pubTnH() {
    static ulong lastPubTs;

    if (millis() - lastPubTs >= 10000) {
        lastPubTs = millis();

        if (WiFi.isConnected() == false) {
            Serial.println("WiFi unavailable during publish.");
            return;
        }

        if (mqttClient.connected() == false) {
            Serial.println("Mqtt unavailable during publish");
            return;
        }

        char t[16];
        char h[16];

        formatWithFallback(dht.getTemperature(), t, sizeof(t));
        formatWithFallback(dht.getHumidity(), h, sizeof(h));

        char payload[128];
        snprintf(
            payload,
            sizeof(payload),
            R"({"temperature":%s,"humidity":%s})",
            t, h);

        mqttClient.publish("iot-course/volodymyr_1de632fd-de4a-4802-99e0-11c756d002c4/sensors", payload);
    }
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

static void formatWithFallback(const float val, char *buffer, const size_t bufferSize) {
    if (isnan(val)) {
        snprintf(buffer, bufferSize, "null");
    } else {
        snprintf(buffer, bufferSize, "%.1f", val);
    }
}

bool isBtnPressed() {
    static bool btnLastRawState = digitalRead(BTN_PIN);
    static bool btnStableState = btnLastRawState;
    static ulong btnLastChangeTime;

    const bool btnRaw = digitalRead(BTN_PIN);
    if (btnRaw != btnLastRawState) {
        btnLastRawState = btnRaw;
        btnLastChangeTime = millis();
    }

    if (millis() - btnLastChangeTime >= 20) {
        btnStableState = btnRaw;
    }

    const bool r = btnStableState == LOW;
    return r;
}

bool isBtnClicked() {
    static bool prevPressed = false;

    const bool pressed = isBtnPressed();
    const bool clicked = !pressed && prevPressed;

    prevPressed = pressed;

    return clicked;
}
