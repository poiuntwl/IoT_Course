#include <Arduino.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>

constexpr int DHT_PIN = 26;
constexpr int BTN_PIN = 19;

static DHTesp dht;

void setup() {
    Serial.begin(115200);
    dht.setup(DHT_PIN, DHTesp::DHT22);
    pinMode(BTN_PIN, INPUT_PULLUP);
    WiFi.begin("Wokwi-GUEST");
}

static void pubTnH();

static void ensureWifi();

static ulong lastSentTs;
static ulong lastWifiCheckTs;

void loop() {
    ensureWifi();
    pubTnH();
}


static void pubTnH() {
    if (millis() - lastSentTs >= 10000) {
        Serial.println("publish TH");
    }
}

static void ensureWifi() {
    if (WiFi.isConnected()) {
        return;
    }
    if (millis() - lastWifiCheckTs >= 5000) {
        lastWifiCheckTs = millis();
        WiFi.begin();
    }
}
