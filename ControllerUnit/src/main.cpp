#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sMQTTBroker_User.h"
#include "arduino_secrets.h"

// Forward declarations
void printMessageBuffer();
void checkForNewMessages();

// Global objects
sMQTTBroker_User broker;

// Message buffer management
static std::vector<String> messageBuffer;
static constexpr size_t MAX_BUFFER_SIZE = 50;
static constexpr unsigned long BUFFER_PRINT_INTERVAL = 60000; // 1 minute

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
    checkForNewMessages();
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

void addMessageToBuffer(const String &message)
{
    messageBuffer.push_back(message);

    // Limit the buffer size
    if (messageBuffer.size() > MAX_BUFFER_SIZE)
    {
        messageBuffer.erase(messageBuffer.begin()); // Remove oldest message
    }
}

void checkForNewMessages()
{
    static char prevPayload[256] = {0};
    static unsigned long lastBufferPrint = 0;

    const char *currentPayload = broker.getLastPayload();

    // Check if we have a new message (payload changed)
    if (currentPayload[0] != '\0' && strcmp(prevPayload, currentPayload) != 0)
    {
        Serial.print("New payload received: ");
        Serial.println(currentPayload);

        // Update previous payload
        strncpy(prevPayload, currentPayload, sizeof(prevPayload) - 1);
        prevPayload[sizeof(prevPayload) - 1] = '\0';

        // Add to buffer
        addMessageToBuffer(String(currentPayload));
    }

    // Print buffer periodically
    if (millis() - lastBufferPrint >= BUFFER_PRINT_INTERVAL)
    {
        printMessageBuffer();
        lastBufferPrint = millis();
    }
}