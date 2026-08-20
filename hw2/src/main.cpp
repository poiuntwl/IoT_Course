#include <Arduino.h>
#include <random>

namespace {
    struct SensorData {
        uint8_t temperature;
        uint8_t humidity;
        time_t currentDate;
    };
}

static uint8_t random_uint8_t(uint8_t min, uint8_t max);

static void log_data();

static tm bootDate = {
    .tm_sec = 0, .tm_min = 30, .tm_hour = 19, .tm_mday = 20, .tm_mon = 8 - 1, .tm_year = 2026 - 1900, .tm_wday = 0,
    .tm_yday = 0, .tm_isdst = 0
};

void setup() {
    Serial.begin(115200);
}

static ulong lastLogTime = 0;
static ulong lastHeapLogTime = 0;
static ulong launchTime = millis();
static SensorData sd;

void loop() {
    const unsigned long now = millis();
    if (now - lastLogTime >= 20000) {
        lastLogTime = now;
        sd = {
            .temperature = random_uint8_t(15, 30),
            .humidity = random_uint8_t(30, 65),
            .currentDate = mktime(&bootDate) + static_cast<time_t>(millis() / 1000)
        };

        log_data();
    }

    if (now - lastHeapLogTime >= 60000) {
        lastHeapLogTime = now;
        if (now - launchTime <= 5 * 60 * 1000) {
            Serial.printf("free heap: %u bytes\r\n", ESP.getFreeHeap());
        }
    }
}

static uint8_t random_uint8_t(const uint8_t min, const uint8_t max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

static void log_data() {
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&sd.currentDate));

    Serial.printf("temperature: %uC\thumidity: %u\tdatetime: %s\r\n", sd.temperature, sd.humidity, buffer);
}
