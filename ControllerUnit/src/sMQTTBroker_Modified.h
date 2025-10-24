#ifndef SMQTTBROKER_USER_H
#define SMQTTBROKER_USER_H

#include "sMQTTBroker.h"
#include "sMQTTEvent.h"
#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <ctime>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "arduino_secrets.h"

class sMQTTBroker_User : public sMQTTBroker
{
public:
    struct messageEntry
    {
        String topic;
        String payload;
        u16_t msgID;
    };
    bool onEvent(sMQTTEvent *event) override;
    const std::vector<messageEntry> &getMessageBuffer() const { return messageBuffer; }

private:
    double lastLat = 0.0;
    double lastLon = 0.0;
    long lastTst = 0;
    bool haveGPS = false;

    // Internal message buffer
    std::vector<messageEntry> messageBuffer;
    static constexpr size_t MAX_BUFFER_SIZE = 1000;

    // Simple resend queue for failed backend posts
    std::vector<String> resendQueue;
    static constexpr size_t MAX_QUEUE = 1000;

    // Time/NTP
    bool timeInitialized = false;

    bool isSensorsTopic(const std::string &topic) const;
    bool isGpsTopic(const std::string &topic) const;
    void handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc);
    void handleSensorMessage(const std::string &topic, const std::string &payload, uint16_t msgID, ArduinoJson::JsonDocument &doc);
    String constructJson(String &body, const std::string &topic, const std::string &payload, uint16_t msgID, ArduinoJson::JsonDocument &doc);
    void addGpsDataAndTimestamp(ArduinoJson::JsonDocument &doc) const;
    void ensureTimeInitialized();
    String currentIsoTimestamp() const;
    bool postToBackend(const String &body);
    void flushResendQueue();
    void handleMessageBuffer(const std::string &topic, const std::string &payload, uint16_t msgID);
};

#endif // SMQTTBROKER_USER_H
