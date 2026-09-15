#pragma once

const char WIFI_SSID[] = "YOUR_WIFI";
const char WIFI_PASSWORD[] = "YOUR_PASSWORD";

const char AWS_IOT_ENDPOINT[] =
    "xxxxxxxxxxxxxx-ats.iot.eu-central-1.amazonaws.com";

static const char AWS_ROOT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";

static const char AWS_DEVICE_CERT[] = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";

static const char AWS_PRIVATE_KEY[] = R"EOF(
-----BEGIN PRIVATE KEY-----
...
-----END PRIVATE KEY-----
)EOF";