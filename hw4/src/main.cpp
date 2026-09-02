#include <Arduino.h>
#include <DHTesp.h>

constexpr int DHT_PIN = 26;
constexpr int BTN_PIN = 19;

static DHTesp dht;

void setup() {
    Serial.begin(115200);
    dht.setup(DHT_PIN, DHTesp::DHT22);
    pinMode(BTN_PIN, INPUT_PULLUP);
}

void loop() {
    Serial.println(dht.getHumidity());
    delay(1000);
}
