#include <Arduino.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <WiFi.h>

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
static ulong lastWifiAttemptTimestamp;
static bool wifiWasConnected;
static bool wifiUnavailableReported;

static const char *WIFI_SSID = "Wokwi-GUEST";
static const char *WIFI_PASSWORD = "";
static const char *HTTP_URL = "http://httpbin.org/post";

static bool isBtnPressed();

static bool isBtnClicked(bool currentState);

static bool catchReadStateChange();

static void lightLed(int brightness);

static void printSensors();

static void connectWifi();

static void maintainWifi();

static void publish();

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    btnLastRawState = digitalRead(BUTTON_PIN);
    btnStableState = btnLastRawState;

    connectWifi();
}

void loop() {
    maintainWifi();

    const ulong now = millis();
    // start countdown whenever we start monitoring
    if (catchReadStateChange()) {
        lastSensorReadTimestamp = now;
        lastSensorPublishTimestamp = now;
    }

    if (readState == MONITOR && now - lastSensorReadTimestamp >= SENSOR_PRINT_INTERVAL) {
        brightness = analogRead(LDR_PIN);
        lightLed(brightness);

        temperature = dht.readTemperature();
        humidity = dht.readHumidity();

        if (isnan(temperature)) {
            Serial.println("DHT22 temperature reading failed: NaN");
        }
        if (isnan(humidity)) {
            Serial.println("DHT22 humidity reading failed: NaN");
        }

        lastSensorReadTimestamp = now;

        if (readState == MONITOR) {
            printSensors();
        }
    }

    if (readState == MONITOR && now - lastSensorPublishTimestamp >= SENSOR_PUBLISH_INTERVAL) {
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

void connectWifi() {
    Serial.println("Connecting to Wi-Fi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttemptTimestamp = millis();
}

void maintainWifi() {
    const ulong now = millis();
    const bool wifiConnected = WiFi.status() == WL_CONNECTED;

    if (wifiConnected) {
        if (!wifiWasConnected) {
            Serial.printf(
                "Wi-Fi connected. IP: %s\n",
                WiFi.localIP().toString().c_str()
            );
        }
        wifiWasConnected = true;
        wifiUnavailableReported = false;
        return;
    }

    if (wifiWasConnected) {
        Serial.println("Wi-Fi disconnected");
    } else if (!wifiUnavailableReported) {
        Serial.println("Wi-Fi unavailable");
    }
    wifiWasConnected = false;
    wifiUnavailableReported = true;

    if (now - lastWifiAttemptTimestamp >= WIFI_RECHECK_INTERVAL) {
        Serial.println("Wi-Fi unavailable; retrying connection");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        lastWifiAttemptTimestamp = now;
    }
}

void publish() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("HTTP POST network error: Wi-Fi unavailable");
        return;
    }

    char temperatureJson[16];
    char humidityJson[16];
    if (isnan(temperature)) {
        strcpy(temperatureJson, "null");
    } else {
        snprintf(temperatureJson, sizeof(temperatureJson), "%.1f", temperature);
    }
    if (isnan(humidity)) {
        strcpy(humidityJson, "null");
    } else {
        snprintf(humidityJson, sizeof(humidityJson), "%.1f", humidity);
    }

    char payload[128];
    snprintf(
        payload,
        sizeof(payload),
        R"({"temperature":%s,"humidity":%s,"brightness":%d})",
        temperatureJson,
        humidityJson,
        brightness
    );

    HTTPClient http;
    if (!http.begin(HTTP_URL)) {
        Serial.println("HTTP POST network error: could not initialize HTTP client");
        http.end();
        return;
    }

    http.addHeader("Content-Type", "application/json");
    const int httpStatus = http.POST(payload);

    if (httpStatus >= 200 && httpStatus < 300) {
        Serial.printf("HTTP POST succeeded: status=%d\n", httpStatus);
        Serial.println("httpbin response body:");
        Serial.println(http.getString());
    } else if (httpStatus > 0) {
        Serial.printf("HTTP POST server error: status=%d\n", httpStatus);
    } else {
        Serial.printf(
            "HTTP POST network error: %s\n",
            http.errorToString(httpStatus).c_str()
        );
    }

    http.end();
}
