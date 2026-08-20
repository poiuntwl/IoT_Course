#include <Arduino.h>
#include <random>

namespace {
    struct SensorData {
        uint8_t temperature;
        uint8_t humidity;
        time_t timestamp;
    };
}

static uint8_t random_uint8_t(uint8_t min, uint8_t max);

void setup() {
    Serial.begin(115200);
}

void loop() {
    const SensorData sd = {
        .temperature = random_uint8_t(15, 30),
        .humidity = random_uint8_t(30, 65),
        .timestamp = time(nullptr)
    };

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&sd.timestamp));

    Serial.printf("temperature: %uC\thumidity: %u%\tdatetime: %s\r\n", sd.temperature, sd.humidity, buffer);
    delay(1000);
}

static uint8_t random_uint8_t(const uint8_t min, const uint8_t max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<uint8_t> dist(min, max);
    return dist(gen);
}
