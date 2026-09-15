#include <Arduino.h>

const char AWS_IOT_ENDPOINT[] = "a3uy4feb9m7i98-ats.iot.eu-central-1.amazonaws.com";

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  // mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}