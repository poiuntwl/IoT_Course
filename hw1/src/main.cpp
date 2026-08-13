#include <Arduino.h>
#define LED 2
#define EXT_LED 4

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    pinMode(EXT_LED, OUTPUT);
}

void loop() {
    Serial.println("Hello world");
    digitalWrite(EXT_LED, HIGH);
    delay(100);
    digitalWrite(EXT_LED, LOW);
    delay(100);
}
