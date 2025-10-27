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
#define AP_IP 192, 168, 10, 1
#define AP_SUBNET 255, 255, 255, 0

#define Seconds *1000
static constexpr unsigned long DIAGNOSTICS_PRINTING_INTERVAL = 30 Seconds; // Print diagnostic information every 30 seconds
static constexpr unsigned long BUFFER_PRINT_INTERVAL = 60 Seconds;         // Print buffer every 60 seconds
static constexpr unsigned long GPS_CLEAR_INTERVAL = 120 Seconds;           // Clear GPS data if no updates for 2 minutes

void startNetworkingInterfaces();
void enable_WiFi_STA();
void enable_WiFi_AP();
void printMessageBuffer();
void printConnectivityDiagnostics();
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
    else
        Broker.update();

#ifdef ENABLE_BUFFER_LOGGING
    // Print buffer periodically
    static unsigned long lastBufferPrint = 0;
    if (millis() - lastBufferPrint >= BUFFER_PRINT_INTERVAL)
    {
        printMessageBuffer();
        lastBufferPrint = millis();
    }
#endif

    // Periodically clears lat and lon if no GPS updates received
    static unsigned long lastGpsReset = 0;
    if (millis() - lastGpsReset >= GPS_CLEAR_INTERVAL)
    {
        Broker.resetGPSCoordinates();
        lastGpsReset = millis();
    }

    // Print diagnostics info to terminal periodically
    static unsigned long lastDiagPrint = 0;
    if (millis() - lastDiagPrint >= DIAGNOSTICS_PRINTING_INTERVAL)
    {
        printConnectivityDiagnostics();
        printFreeMemory();
        lastDiagPrint = millis();
    }
}

#ifdef ENABLE_BUFFER_LOGGING
void printMessageBuffer()
{
    LOG_MESSAGE("\n--- Message Buffer Dump ---\n");
    for (size_t i = 0; i < Broker.getMessageBuffer().size(); ++i)
    {
        const auto &buffer = Broker.getMessageBuffer()[i];
        LOG_MESSAGE("[%zu] Topic: %s\nPayload:\n%s\nMessage ID: %u\n", i + 1, buffer.topic.c_str(), buffer.payload.c_str(), buffer.msgID);
    }
    LOG_MESSAGE("--- End of Buffer ---\n");
}
#endif

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
    if (WiFi.status() != WL_CONNECTED)
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

void printConnectivityDiagnostics()
{
    LOG_MESSAGE("\n--- Connectivity Diagnostics ---\n");

    // STA Info
    bool WifiStatus = (WiFi.status() == WL_CONNECTED);
    LOG_MESSAGE("WiFi STA Status: %s\n", WifiStatus ? "Connected" : "Disconnected");
    if (WifiStatus)
    {
        LOG_MESSAGE("SSID: %s\n", WiFi.SSID().c_str());
        LOG_MESSAGE("IP Address: %s\n", WiFi.localIP().toString().c_str());
        LOG_MESSAGE("Signal Strength (RSSI): %d dBm\n", WiFi.RSSI());
    }
    else
    {
        LOG_MESSAGE("Not connected to any network.\n");
    }

    // AP Info
    int apClients = WiFi.softAPgetStationNum();
    LOG_MESSAGE("WiFi AP Status: %d\n", apClients > 0 ? " clients Connected" : "No Clients Connected");
    LOG_MESSAGE("AP SSID: %s\n", WiFi.softAPSSID().c_str());
    LOG_MESSAGE("AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    LOG_MESSAGE("Number of clients connected to AP: %d\n", apClients);

    // General Info
    LOG_MESSAGE("MQTT Clients: %d\n", Broker.getClientCount());

    LOG_MESSAGE("--- End of Diagnostics ---\n");
}

void printFreeMemory()
{
    size_t freeMem = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    LOG_MESSAGE("Free Memory: %zu Bytes.\n", freeMem);
}
