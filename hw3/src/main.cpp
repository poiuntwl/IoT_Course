#include <Arduino.h>
#include <DHT.h>

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

DHT dht(DHT22_PIN, DHT22);

namespace {
    enum READ_STATE { SILENCE, MONITOR };
}

static bool btnLastRawState;
static bool btnStableState;
static ulong btnLastChangeTime;
static READ_STATE readState = SILENCE;
static bool btnPrevPressed = false;

static ulong lastSensorReadTimestamp;
static int brightness;
static float temperature;
static float humidity;

static bool isBtnPressed();

static bool isBtnClicked(bool currentState);

static bool catchReadStateChange();

static void lightLed(int brightness);

static void printSensors();

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(DHT22_PIN, INPUT);

    btnLastRawState = digitalRead(BUTTON_PIN);
    btnStableState = btnLastRawState;
}

void loop() {
    if (catchReadStateChange()) {
        lastSensorReadTimestamp = millis();
    }

    if (millis() - lastSensorReadTimestamp > 5000) {
        brightness = analogRead(LDR_PIN);
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
        lightLed(brightness);

        lastSensorReadTimestamp = millis();

        if (readState == MONITOR) {
            printSensors();
        }
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
