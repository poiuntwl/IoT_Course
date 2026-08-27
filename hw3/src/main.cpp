#include <Arduino.h>

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
#define LIGHT_THRESHOLD 100

namespace {
    enum READ_STATE { SILENCE, MONITOR };
}

static bool btnLastRawState;
static bool btnStableState;
static ulong btnLastChangeTime;
static READ_STATE readState = SILENCE;
static bool btnPrevPressed = false;
static ulong lastSensorReadTimestamp;

static bool isBtnPressed();

static bool isBtnClicked(bool currentState);

static void catchReadStateChange();

static void lightLed(int brightness);

void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    btnLastRawState = digitalRead(BUTTON_PIN);
    btnStableState = btnLastRawState;
}

void loop() {
    catchReadStateChange();

    if (millis() - lastSensorReadTimestamp > 1000) {
        lastSensorReadTimestamp = millis();

        if (readState == MONITOR) {
            // send
        }

        int brightness = analogRead(LDR_PIN);
        lightLed(brightness);
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

static void catchReadStateChange() {
    bool btnPressed = isBtnPressed();
    if (isBtnClicked(btnPressed)) {
        readState = readState == SILENCE ? MONITOR : SILENCE;
    }
    btnPrevPressed = btnPressed;
}

void lightLed(int brightness) {
    Serial.println(brightness);
    if (brightness >= LIGHT_THRESHOLD) {
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}
