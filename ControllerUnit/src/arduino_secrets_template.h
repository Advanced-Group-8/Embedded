#ifndef ARDUINO_SECRET_H
#define ARDUINO_SECRET_H

// User defined info
#define AZURE_BACKEND_URL "YOUR_AZURE_BACKEND_URL"
#define AZURE_BACKEND_URL_PACKAGE_TRACKING "YOUR_AZURE_BACKEND_URL_PACKAGE_TRACKING"
#define AZURE_BACKEND_URL_SIGNUP "YOUR_AZURE_BACKEND_URL_SIGNUP"
#define AZURE_BACKEND_URL_SIGNIN "YOUR_AZURE_BACKEND_URL_SIGNIN"
#define AZURE_JWT_TOKEN "YOUR_JWT_TOKEN_HERE"

// WiFi settings
constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

// AP WiFi settings
constexpr char AP_WIFI_SSID[] = "YOUR_AP_WIFI_SSID";
constexpr char AP_WIFI_PASSWORD[] = "YOUR_AP_WIFI_PASSWORD";

// MQTT Broker settings
constexpr char MQTT_BROKER[] = "MQTT_BROKER_IP";
constexpr unsigned int MQTT_PORT = 1883;

// Backend endpoint to forward enriched messages (HTTP/HTTPS URL)
// Example: "http://192.168.0.10:8080/ingest" or "https://example.com/ingest"
#ifndef BACKEND_URL
#define BACKEND_URL "YOU_BACKEND_URL"
#endif

#endif // ARDUINO_SECRET_H
