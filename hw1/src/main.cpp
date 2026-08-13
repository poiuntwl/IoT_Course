#include <Arduino.h>

const int LedPin = 4;
const int DotDurationMs = 200;
const int DashDurationMs = 600;

void SendSymbol(char symbol)
{
    Serial.print(symbol);

    digitalWrite(LedPin, HIGH);

    if (symbol == '.')
    {
        delay(DotDurationMs);
    }
    else
    {
        delay(DashDurationMs);
    }

    digitalWrite(LedPin, LOW);
}

void SendLetter(const char* code)
{
    for (int i = 0; code[i] != '\0'; i++)
    {
        SendSymbol(code[i]);

        if (code[i + 1] != '\0')
        {
            delay(DotDurationMs);
        }
    }

    delay(DotDurationMs * 3);
}

void setup()
{
    Serial.begin(115200);
    pinMode(LedPin, OUTPUT);
}

void loop()
{
    SendLetter("...");
    SendLetter("---");
    SendLetter("...");

    Serial.println();
}