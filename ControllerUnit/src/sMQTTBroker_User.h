#ifndef SMQTTBROKER_USER_H
#define SMQTTBROKER_USER_H

#include "sMQTTBroker.h"
#include "sMQTTEvent.h"
#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

class sMQTTBroker_User : public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event) override;
    const char *getLastPayload() const { return lastReceivedPayload; }

private:
    char lastReceivedPayload[512] = {0};
    double lastLat = 0.0;
    double lastLon = 0.0;
    long lastTst = 0;
    bool haveGPS = false;

    // Simple resend queue for failed backend posts
    std::vector<String> resendQueue;
    static constexpr size_t MAX_QUEUE = 20;

    // Time/NTP
    bool timeInitialized = false;

    // Helpers for clarity and separation of concerns
    bool isSensorsTopic(const std::string &topic) const;
    bool isGpsTopic(const std::string &topic) const;
    void handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc, bool isJson);
    void handleSensorMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc, bool isJson);
    void enrichWithGpsAndTimestamp(ArduinoJson::JsonDocument &doc) const;
    void ensureTimeInitialized();
    String currentIsoTimestamp() const;
    bool postToBackend(const String &body);
    void flushResendQueue();
};

#endif // SMQTTBROKER_USER_H
