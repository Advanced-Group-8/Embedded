#ifndef ARDUINO_SECRET_H
#define ARDUINO_SECRET_H

// WiFi settings
constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
constexpr char MQTT_BROKER[] = "MQTT_BROKER_IP";
constexpr unsigned int MQTT_PORT = 1883;

// Backend endpoint to forward enriched messages (HTTP/HTTPS URL)
// Example: "http://192.168.0.10:8080/ingest" or "https://example.com/ingest"
#ifndef BACKEND_URL
#define BACKEND_URL "YOU_BACKEND_URL"
#endif

#endif // ARDUINO_SECRET_H
