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

class sMQTTBroker_User : public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event) override;
    struct messageEntry
    {
        String topic;
        String payload;
        uint16_t msgID;
    };
    const std::deque<messageEntry> &getMessageBuffer() const { return messageBuffer; }
    void resetGPSCoordinates();
    int getClientCount() const;

private:
    HTTPClient http;

    // Time/NTP
    bool timeInitialized = false;
    // GPS data
    double lastLat = 0.0;
    double lastLon = 0.0;
    long lastTst = 0;
    bool haveGPS = false;

#ifdef ENABLE_LOGGING
    // Internal message buffer
    std::deque<messageEntry> messageBuffer;
    static constexpr size_t MAX_BUFFER_SIZE = 5000;
#endif

    // Simple resend queue for failed backend posts
    std::deque<String> resendQueue;
    static constexpr size_t MAX_QUEUE = 5000;

    bool isSensorsTopic(const std::string &topic) const;
    bool isGpsTopic(const std::string &topic) const;
    void handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    void handleSensorMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    String constructJson(String &body, const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    void addGpsDataAndTimestamp(ArduinoJson::JsonDocument &doc) const;
    void ensureTimeInitialized();
    String currentIsoTimestamp() const;
    bool postToBackend(const String &body);
    void processQueue();
    void handleMessageBuffer(const std::string &topic, const std::string &payload, uint16_t msgID);
};

#endif // SMQTTBROKER_USER_H
