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
void enable_WiFi_STA();
void enable_WiFi_AP();
void printMessageBuffer();
void printFreeMemory();

sMQTTBroker_User Broker;

void setup()
{
    Serial.begin(115200);
    startNetworkingInterfaces();
    Broker.init(MQTT_PORT);
}

void loop()
{
    Broker.update();

    // Print buffer periodically
    static unsigned long lastBufferPrint = 0;
    if (millis() - lastBufferPrint >= BUFFER_PRINT_INTERVAL)
    {
        printMessageBuffer();
        lastBufferPrint = millis();
    }

    // Print free memory periodically
    static unsigned long lastFreeMemoryPrint = 0;
    if (millis() - lastFreeMemoryPrint >= FREE_MEMORY_PRINT_INTERVAL)
    {
        printFreeMemory();
        lastFreeMemoryPrint = millis();
    }

    delay(3 Seconds);
}

void printMessageBuffer()
{
    LOG_MESSAGE("\n--- Message Buffer Dump ---\n");
    for (size_t i = 0; i < Broker.getMessageBuffer().size(); ++i)
    {
        const auto &buffer = Broker.getMessageBuffer()[i];
        LOG_MESSAGE("[%zu] Topic: %s\nMessage ID: %u\nPayload:\n %s\n ", i + 1, buffer.topic.c_str(), buffer.msgID, buffer.payload.c_str());
    }
    LOG_MESSAGE("--- End of Buffer ---\n");
}

void startNetworkingInterfaces()
{
    WiFi.mode(WIFI_AP_STA);
    enable_WiFi_AP();
    enable_WiFi_STA();
}

void enable_WiFi_STA()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setHostname(WiFi.macAddress().c_str());
    LOG_MESSAGE("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED)
        delay(1 Seconds);
    LOG_MESSAGE("Connection established!\nIP address: %s\n", WiFi.localIP().toString().c_str());
}

void enable_WiFi_AP()
{
    IPAddress AP_IP(192, 168, 10, 1);
    IPAddress AP_Subnet(255, 255, 255, 0);
    IPAddress LEASE_START(192, 168, 10, 2);
    WiFi.softAPConfig(AP_IP, AP_IP, AP_Subnet);
    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASSWORD, 1, 1, 20, false);
    LOG_MESSAGE("AP started!\nAP IP address: %s\n", WiFi.softAPIP().toString().c_str());
}

void printFreeMemory()
{
    size_t freeMem = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    LOG_MESSAGE("Free Memory: %zu Bytes.\n", freeMem);
}
