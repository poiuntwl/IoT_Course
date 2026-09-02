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

static void pubTnH();

static ulong lastSentTs;

void loop() {
    if (millis() - lastSentTs > 10000) {
        pubTnH();
    }
}


static void pubTnH() {
}
