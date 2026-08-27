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

void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    bool pressed = digitalRead(BUTTON_PIN) == LOW;

    if (pressed) {
        Serial.println("Pressed");
    } else {
        Serial.println("Not Pressed");
    }

    delay(100);
}
