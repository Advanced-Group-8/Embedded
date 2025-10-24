#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sMQTTBroker_Modified.h"
#include "arduino_secrets.h"

#ifdef ENABLE_LOGGING
#define LOG_MESSAGE(...) Serial.printf(__VA_ARGS__)
#else
#define LOG_MESSAGE(...) \
    do                   \
    {                    \
    } while (0)
#endif
#define Seconds *1000
#define AP_IP 192, 168, 10, 1
#define AP_SUBNET 255, 255, 255, 0

static constexpr unsigned long BUFFER_PRINT_INTERVAL = 60 Seconds;      // Print buffer every 60 seconds
static constexpr unsigned long FREE_MEMORY_PRINT_INTERVAL = 30 Seconds; // Print free memory every 30 seconds
static constexpr unsigned long GPS_CLEAR_INTERVAL = 120 Seconds;        // Clear GPS data if no updates for 2 minutes
static constexpr unsigned long DIAGNOSE_PRINTING_INTERVAL = 60 Seconds; // Print diagnostic information every 1 minute

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
    if (WiFi.status() != WL_CONNECTED || WiFi.softAPIP() != IPAddress(AP_IP))
    {
        LOG_MESSAGE("Either STA or AP is down, restarting interfaces...\n");
        startNetworkingInterfaces();
    }

    Broker.update();

    static unsigned long lastDiagPrint = 0;
    if (millis() - lastDiagPrint >= DIAGNOSE_PRINTING_INTERVAL)
    {
        LOG_MESSAGE("WiFi.status: %d\n", WiFi.status());
        LOG_MESSAGE("Local IP: %s\n", WiFi.localIP().toString().c_str());
        LOG_MESSAGE("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        LOG_MESSAGE("MQTT clients: %d\n", Broker.getClientCount());
        lastDiagPrint = millis();
    }

    // Print buffer periodically
    // static unsigned long lastBufferPrint = 0;
    // if (millis() - lastBufferPrint >= BUFFER_PRINT_INTERVAL)
    // {
    //     printMessageBuffer();
    //     lastBufferPrint = millis();
    // }

    // Print free memory periodically
    static unsigned long lastFreeMemoryPrint = 0;
    if (millis() - lastFreeMemoryPrint >= FREE_MEMORY_PRINT_INTERVAL)
    {
        printFreeMemory();
        lastFreeMemoryPrint = millis();
    }

    // Periodically clears lat and lon if no GPS updates received
    static unsigned long lastGpsCheck = 0;
    if (millis() - lastGpsCheck >= GPS_CLEAR_INTERVAL)
    {
        Broker.resetGPSCoordinates();
        lastGpsCheck = millis();
    }
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
    delay(1 Seconds);
    enable_WiFi_STA();
    delay(1 Seconds);
}

void enable_WiFi_STA()
{
    WiFi.disconnect();
    delay(1 Seconds);
    LOG_MESSAGE("Setting hostname to %s\n", WiFi.macAddress().c_str());
    WiFi.setHostname(WiFi.macAddress().c_str());
    LOG_MESSAGE("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(1 Seconds);
    }
    LOG_MESSAGE("Connection established!\nIP address: %s\n", WiFi.localIP().toString().c_str());
}

void enable_WiFi_AP()
{
    WiFi.softAPdisconnect();
    delay(1 Seconds);
    IPAddress _AP_IP(AP_IP);
    IPAddress _AP_Subnet(AP_SUBNET);
    IPAddress LEASE_START(192, 168, 10, 2);
    WiFi.softAPConfig(_AP_IP, _AP_IP, _AP_Subnet);
    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASSWORD, 1, 1, 20, false);
    LOG_MESSAGE("AP started!\nAP IP address: %s\n", WiFi.softAPIP().toString().c_str());
}

void printFreeMemory()
{
    size_t freeMem = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    LOG_MESSAGE("Free Memory: %zu Bytes.\n", freeMem);
}
