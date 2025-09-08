#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sMQTTBroker_User.h"
#include "arduino_secrets.h"

void printMessageBuffer();

sMQTTBroker_User broker;
static std::vector<String> messageBuffer;

void setup()
{
    Serial.begin(115200);
    delay(5000);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
    }
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    broker.init(MQTT_PORT);
}

void loop()
{
    broker.update();

    static char prevPayload[256] = {0};
    const char *currentPayload = broker.getLastPayload(); // Get the current payload

    // Check if the payload has changed
    if (strcmp(prevPayload, currentPayload) != 0)
    {
        Serial.print("New payload received: ");
        Serial.println(currentPayload);
        strncpy(prevPayload, currentPayload, sizeof(prevPayload) - 1); // Copy current payload to previous payload
        prevPayload[sizeof(prevPayload) - 1] = '\0';

        // Store the new message in the buffer if it's changed
        messageBuffer.push_back(String(currentPayload));
    }

    // Limit the buffer size
    const size_t MAX_BUFFER_SIZE = 50;
    if (messageBuffer.size() > MAX_BUFFER_SIZE)
    {
        messageBuffer.erase(messageBuffer.begin()); // Remove oldest when buffer exceeds max size
    }

    // Print the buffer every 1 minute
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 60000)
    {
        printMessageBuffer();
        lastPrint = millis();
    }

    delay(1000);
}

/*
Steps:
In your sMQTTBroker_User class, add a member variable for lastMsgId (e.g., uint16_t lastMsgId = 0;).
In your onEvent handler, when you receive a publish event, extract the msgId and store it in lastMsgId.
Add a getter: uint16_t getLastMsgId() const { return lastMsgId; }
In your main loop, keep a static prevMsgId and compare it to broker.getLastMsgId(). If it changes, you know a new message arrived, even if the payload is the same.
*/

void printMessageBuffer()
{
    Serial.println("\n--- Message Buffer Dump ---");
    for (size_t i = 0; i < messageBuffer.size(); ++i)
    {
        Serial.print("[");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(messageBuffer[i]);
    }
    Serial.println("--- End of Buffer ---\n");
}