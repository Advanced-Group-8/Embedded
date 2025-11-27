#ifndef SMQTTBROKER_USER_H
#define SMQTTBROKER_USER_H

#include "sMQTTBroker.h"
#include "sMQTTEvent.h"
#include <Arduino.h>
#include <WiFi.h>
#include <deque>
#include <ctime>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "arduino_secrets.h"

class sMQTTBroker_Modified : public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event) override;
    void resetGPSCoordinates();
    uint16_t getClientCount() const;
    void processQueue();

private:
    // Time/NTP
    bool timeInitialized = false;
    // GPS data
    double lastLat = 0.0;
    double lastLon = 0.0;
    long lastTst = 0;
    bool haveGPS = false;

    // Simple resend queue for failed backend posts
    std::deque<String> messageQueue;
    static constexpr size_t MAX_QUEUE = 900; // 850 have been tested and works fine

    bool isSensorsTopic(const std::string &topic) const;
    bool isGpsTopic(const std::string &topic) const;
    void handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    void handleSensorMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    String constructJson(String &body, const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    void addGpsDataAndTimestamp(ArduinoJson::JsonDocument &doc) const;
    void ensureTimeInitialized();
    String currentIsoTimestamp() const;
    bool postToBackend(const String &body);
};

#endif // SMQTTBROKER_USER_H
