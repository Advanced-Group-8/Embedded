#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sMQTTBroker_Modified.h"
#include "arduino_secrets.h"

#ifdef ENABLE_LOG_MESSAGE
#define LOG_MESSAGE(...) Serial.printf(__VA_ARGS__)
#else
#define LOG_MESSAGE(...) \
    do                   \
    {                    \
    } while (0)
#endif
#define Seconds *1000
#ifdef DEVELOPMENT_BUILD
static constexpr unsigned long BACKEND_UPLOAD_INTERVAL = 10 Seconds;       // Upload to backend every 10 seconds
static constexpr unsigned long DIAGNOSTICS_PRINTING_INTERVAL = 30 Seconds; // Print diagnostic information every 30 seconds
static constexpr unsigned long GPS_CLEAR_INTERVAL = 60 Seconds;            // Clear GPS data if no updates for 60 seconds
#endif

void startNetworkingInterfaces();
void enable_WiFi_STA();
void enable_WiFi_AP();
void uploadToBackend(const unsigned long interval);
#ifdef DEVELOPMENT_BUILD
void printConnectivityDiagnostics();
void printFreeMemory();
#endif

sMQTTBroker_Modified Broker;

void setup()
{
    Serial.begin(115200);
    startNetworkingInterfaces();
    Broker.init(MQTT_PORT);
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_MESSAGE("WiFi STA disconnected, reconnecting...\n");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(5 Seconds);
    }
    else if (WiFi.softAPIP() != IPAddress(AP_IP))
    {
        LOG_MESSAGE("WiFi AP is down, restarting...\n");
        WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASSWORD, 6, 1, 30, false);
        delay(5 Seconds);
    }
    else
        Broker.update();

    uploadToBackend(BACKEND_UPLOAD_INTERVAL);

#ifdef DEVELOPMENT_BUILD
    // Periodically clears lat and lon if no GPS updates received
    static unsigned long lastGpsReset = 0;
    if (millis() - lastGpsReset >= GPS_CLEAR_INTERVAL)
    {
        LOG_MESSAGE("Resetting GPS coordinates.\n");
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
#endif
}

void uploadToBackend(const unsigned long interval)
{
    static unsigned long lastQueueProcess = 0;
    if (millis() - lastQueueProcess >= interval)
    {
        Broker.processQueue();
        lastQueueProcess = millis();
    }
}

void startNetworkingInterfaces()
{
    WiFi.mode(WIFI_AP_STA);
    enable_WiFi_AP();
    enable_WiFi_STA();
}

void enable_WiFi_STA()
{
    WiFi.disconnect();
    delay(5 Seconds);
    LOG_MESSAGE("Setting hostname to %s\n", WiFi.macAddress().c_str());
    WiFi.setHostname(WiFi.macAddress().c_str());
    LOG_MESSAGE("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    uint8_t attempt = 0;
    while (attempt <= 50)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        if (WiFi.waitForConnectResult() == WL_CONNECTED)
        {
            LOG_MESSAGE("WiFi connected successfully!\n");
            break;
        }
        LOG_MESSAGE("WiFi connection attempt %d failed, retrying...\n", ++attempt);
        delay(5 Seconds);
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
    delay(5 Seconds);
    LOG_MESSAGE("AP started!\nAP IP address: %s\n", WiFi.softAPIP().toString().c_str());
}

#ifdef DEVELOPMENT_BUILD
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
        LOG_MESSAGE("Not connected to any network.\n");

    // AP Info
    LOG_MESSAGE("AP SSID: %s\n", WiFi.softAPSSID().c_str());
    LOG_MESSAGE("AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    LOG_MESSAGE("Number of clients connected to AP: %d\n", WiFi.softAPgetStationNum());

    // General Info
    LOG_MESSAGE("MQTT Clients: %d\n", Broker.getClientCount());
    LOG_MESSAGE("--- End of Diagnostics ---\n");
}

void printFreeMemory()
{
    size_t freeMem = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    LOG_MESSAGE("Free Memory: %zu Bytes.\n", freeMem);
}
#endif
