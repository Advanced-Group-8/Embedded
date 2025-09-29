#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sMQTTBroker_User.h"
#include "arduino_secrets.h"

#define LOG_MESSAGE(...) Serial.printf(__VA_ARGS__)
#define Seconds *1000
static constexpr unsigned long BUFFER_PRINT_INTERVAL = 60 Seconds;      // Print buffer every 60 seconds
static constexpr unsigned long FREE_MEMORY_PRINT_INTERVAL = 30 Seconds; // Print free memory every 30 seconds

void startNetworkingInterfaces();
void printMessageBuffer();
void checkForNewMessages();
void printFreeMemory();

sMQTTBroker_User broker;
static std::vector<String> messageBuffer;
static constexpr size_t MAX_BUFFER_SIZE = 100;

void setup()
{
    Serial.begin(115200);
    delay(5000);
    startNetworkingInterfaces();
    broker.init(MQTT_PORT);
}

void loop()
{
    broker.update();
    checkForNewMessages();
    static unsigned long lastFreeMemoryPrint = 0;
    if (millis() - lastFreeMemoryPrint >= FREE_MEMORY_PRINT_INTERVAL)
    {
        printFreeMemory();
        lastFreeMemoryPrint = millis();
    }
    delay(1000);
}

void printMessageBuffer()
{
    LOG_MESSAGE("\n--- Message Buffer Dump ---\n");
    for (size_t i = 0; i < messageBuffer.size(); ++i)
    {
        LOG_MESSAGE("[%zu] %s\n", i + 1, messageBuffer[i].c_str());
    }
    LOG_MESSAGE("--- End of Buffer ---\n");
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
        LOG_MESSAGE("New payload received:\n%s\n", currentPayload);

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

void printFreeMemory()
{
    size_t freeMem = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    LOG_MESSAGE("Free Memory: %zu Bytes.\n", freeMem);
}

void startNetworkingInterfaces()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
    }
    LOG_MESSAGE("Connection established!\nIP address: %s\n", WiFi.localIP().toString().c_str());

    IPAddress AP_IP(192, 168, 10, 1);
    IPAddress AP_Subnet(255, 255, 255, 0);
    IPAddress LEASE_START(192, 168, 10, 2);
    WiFi.softAPConfig(AP_IP, AP_IP, AP_Subnet);
    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASSWORD, 1, 1, 20, false);
    LOG_MESSAGE("AP started!\nAP IP address: %s\n", WiFi.softAPIP().toString().c_str());
}
