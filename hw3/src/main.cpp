#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

/// components:
/// button:         digital input
/// photoresistor:  analog input
/// DHT22:          digital protocol
/// LED:            digital output
///
/// BUTTON:
/// switches between SILENCE and MONITOR
/// uses debounce
/// MONITOR: read each 5s: temperature, humidity, brightness
/// output: serial monitor (TX)
///
/// LED:
/// if brightness < threshold: turn on
///
/// WEB:
/// each 30s: POST httpbin.org/post (or locally set up server)
///
/// EDGE CASES:
/// WI-FI unavailable: output message, continue
/// sensor errors: output message, continue

#define BUTTON_PIN 4
#define LDR_PIN 34
#define LED_PIN 26
#define DHT22_PIN 27
#define LIGHT_THRESHOLD 100
#define SENSOR_PRINT_INTERVAL 5000
#define SENSOR_PUBLISH_INTERVAL 30000
#define WIFI_RECHECK_INTERVAL 30000
#define MQTT_RETRY_INTERVAL 30000

namespace {
    enum READ_STATE { SILENCE, MONITOR };
}

static bool btnLastRawState;
static bool btnStableState;
static ulong btnLastChangeTime;
static READ_STATE readState = SILENCE;
static bool btnPrevPressed = false;

static DHT dht(DHT22_PIN, DHT22);
static ulong lastSensorReadTimestamp;
static ulong lastSensorPublishTimestamp;
static int brightness;
static float temperature;
static float humidity;

static const char *WIFI_SSID = "Wokwi-GUEST";
static const char *WIFI_PASSWORD = "";

static const char *MQTT_HOST = "broker.hivemq.com";
static const uint16_t MQTT_PORT = 1883;
static ulong lastMqttRetry;

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static bool isBtnPressed();

static bool isBtnClicked(bool currentState);

static bool catchReadStateChange();

static void lightLed(int brightness);

static void printSensors();

static bool connectWifi();

static bool connectMqtt();

static void publish();

static void doctorMqtt(ulong now);

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(DHT22_PIN, INPUT);

    btnLastRawState = digitalRead(BUTTON_PIN);
    btnStableState = btnLastRawState;

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    connectWifi();
    connectMqtt();
}

void loop() {
    const ulong now = millis();
    // start countdown whenever we start monitoring
    if (catchReadStateChange()) {
        lastSensorReadTimestamp = now;
        lastSensorPublishTimestamp = now;
    }

    doctorMqtt(now);

    if (now - lastSensorReadTimestamp >= SENSOR_PRINT_INTERVAL) {
        brightness = analogRead(LDR_PIN);
        lightLed(brightness);

        temperature = dht.readTemperature();
        humidity = dht.readHumidity();

        lastSensorReadTimestamp = now;

        if (readState == MONITOR) {
            printSensors();
        }
    }

    if (now - lastSensorPublishTimestamp >= SENSOR_PUBLISH_INTERVAL) {
        lastSensorPublishTimestamp = now;

        publish();
    }
}

bool isBtnPressed() {
    const bool btnRaw = digitalRead(BUTTON_PIN);
    if (btnRaw != btnLastRawState) {
        btnLastRawState = btnRaw;
        btnLastChangeTime = millis();
    }

    if (millis() - btnLastChangeTime >= 20) {
        btnStableState = btnRaw;
    }

    return btnStableState == LOW;
}

static bool isBtnClicked(const bool currentState) {
    return !currentState && btnPrevPressed;
}

static bool catchReadStateChange() {
    bool changed = false;
    bool btnPressed = isBtnPressed();
    if (isBtnClicked(btnPressed)) {
        readState = readState == SILENCE ? MONITOR : SILENCE;
        changed = true;
    }
    btnPrevPressed = btnPressed;

    return changed;
}

void lightLed(int brightness) {
    if (brightness >= LIGHT_THRESHOLD) {
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}

void printSensors() {
    Serial.printf(
        "Temp: %.1f C, Humidity: %.1f %%, Brightness (reversed): %d\r\n",
        temperature,
        humidity,
        brightness
    );
}

bool connectWifi() {
    Serial.print("Connecting to Wi-Fi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    ulong startedAt = millis();

    while (
        WiFiClass::status() != WL_CONNECTED &&
        millis() - startedAt < 10000
    ) {
        delay(250);
        Serial.print(".");
    }

    if (WiFiClass::status() != WL_CONNECTED) {
        Serial.println("\nWi-Fi unavailable");
        return false;
    }

    Serial.printf(
        "\nWi-Fi connected. IP: %s\n",
        WiFi.localIP().toString().c_str()
    );

    return true;
}

bool connectMqtt() {
    if (WiFiClass::status() != WL_CONNECTED) {
        return false;
    }

    Serial.print("Connecting to MQTT... ");

    if (!mqttClient.connect("esp32-client")) {
        Serial.printf(
            "failed, state=%d\n",
            mqttClient.state()
        );

        return false;
    }

    Serial.println("connected");
    return true;
}

void publish() {
    char payload[128];
    snprintf(
        payload,
        sizeof(payload),
        R"({"temperature":%.1f,"humidity":%.1f,"brightness":%d})",
        temperature,
        humidity,
        brightness
    );

    mqttClient.publish(
        "55debed3-a003-419e-bf0f-255b4293f0b5/esp32/sensors",
        payload
    );
}

static void doctorMqtt(const ulong now) {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }

    if (!mqttClient.connected()
        && millis() - lastMqttRetry >= MQTT_RETRY_INTERVAL) {
        connectMqtt();
        lastMqttRetry = now;
    }
}
