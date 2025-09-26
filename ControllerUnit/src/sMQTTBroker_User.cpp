// Handles incoming publish events: clearly separates GPS updates vs Sensor posts
#include "sMQTTBroker_User.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "arduino_secrets.h"
#include <ctime>

bool sMQTTBroker_User::onEvent(sMQTTEvent *event)
{
    if (event->Type() != Public_sMQTTEventType)
        return true;

    auto *pubEvent = static_cast<sMQTTPublicClientEvent *>(event);
    const std::string &topic = pubEvent->Topic();
    const std::string &payload = pubEvent->Payload();

    SMQTT_LOGD("Received topic: %s\n", topic.c_str());
    SMQTT_LOGD("Payload: %s\n", payload.c_str());

    // Always keep last payload for main.cpp
    strncpy(this->lastReceivedPayload, payload.c_str(), sizeof(this->lastReceivedPayload) - 1);
    this->lastReceivedPayload[sizeof(this->lastReceivedPayload) - 1] = '\0';

    // Parse JSON if applicable
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    bool isJson = !err;
    if (!isJson)
        SMQTT_LOGD("JSON parse failed: %s\n", err.c_str());

    // Check type and dispatch
    if (isGpsTopic(topic))
    {
        handleGpsMessage(topic, payload, doc, isJson);
    }
    else if (isSensorsTopic(topic))
    {
        handleSensorMessage(topic, payload, doc, isJson);
    }

    flushResendQueue();
    return true;
}

bool sMQTTBroker_User::isSensorsTopic(const std::string &topic) const
{
    return topic.rfind("sensors/", 0) == 0;
}

bool sMQTTBroker_User::isGpsTopic(const std::string &topic) const
{
    return topic.rfind("owntracks/", 0) == 0;
}

void sMQTTBroker_User::handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc, bool isJson)
{
    if (!isJson)
    {
        SMQTT_LOGD("GPS message is not JSON; ignoring\n");
        return;
    }
    if (doc["lat"].is<double>() && doc["lon"].is<double>())
    {
        lastLat = doc["lat"].as<double>();
        lastLon = doc["lon"].as<double>();
        lastTst = doc["tst"].is<long>() ? doc["tst"].as<long>() : lastTst;
        haveGPS = true;
        SMQTT_LOGD("GPS updated: lat=%.6f lon=%.6f tst=%ld\n", lastLat, lastLon, lastTst);
    }
    else
    {
        SMQTT_LOGD("GPS JSON missing lat/lon; ignoring\n");
    }
}

void sMQTTBroker_User::handleSensorMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc, bool isJson)
{
    SMQTT_LOGD("Sensor payload received for topic %s\n", topic.c_str());
    String body;
    if (isJson)
    {
        enrichWithGpsAndTimestamp(doc);
        SMQTT_LOGD("Enriched with gps+timestamp\n");
        // Build body
        JsonDocument out;
        out["topic"] = topic.c_str();
        out["controller"] = WiFi.getHostname() ? WiFi.getHostname() : WiFi.macAddress().c_str();
        out["payload"] = doc;
        serializeJsonPretty(out, body);
    }
    else
    {
        JsonDocument out;
        out["topic"] = topic.c_str();
        out["controller"] = WiFi.getHostname() ? WiFi.getHostname() : WiFi.macAddress().c_str();
        out["payload"] = payload.c_str();
        serializeJsonPretty(out, body);
    }

    if (!postToBackend(body))
    {
        if (resendQueue.size() < MAX_QUEUE)
            resendQueue.push_back(body);
    }
}

void sMQTTBroker_User::enrichWithGpsAndTimestamp(ArduinoJson::JsonDocument &doc) const
{
    if (haveGPS)
    {
        JsonObject gps = doc["gps"].to<JsonObject>();
        gps["lat"] = lastLat;
        gps["lon"] = lastLon;
    }
    doc["Timestamp"] = currentIsoTimestamp();
}

void sMQTTBroker_User::ensureTimeInitialized()
{
    if (timeInitialized)
        return;
    configTzTime("pool.ntp.org", "time.nist.gov");
    timeInitialized = true;
}

String sMQTTBroker_User::currentIsoTimestamp() const
{
    time_t now;
    time(&now);
    struct tm tmInfo;
    gmtime_r(&now, &tmInfo);
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             tmInfo.tm_year + 1900,
             tmInfo.tm_mon + 1,
             tmInfo.tm_mday,
             tmInfo.tm_hour,
             tmInfo.tm_min,
             tmInfo.tm_sec);
    return String(buf);
}

bool sMQTTBroker_User::postToBackend(const String &body)
{
    if (!WiFi.isConnected())
    {
        SMQTT_LOGD("WiFi not connected; skipping backend POST\n");
        return false;
    }
    ensureTimeInitialized();
    HTTPClient http;
    String url = String(BACKEND_URL);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    SMQTT_LOGD("Posting to backend: %s\n", body.c_str());
    int code = http.POST(body);
    if (code > 0)
    {
        SMQTT_LOGD("Backend POST status: %d\n", code);
        http.end();
        return true;
    }
    SMQTT_LOGD("Backend POST failed: %s\n", http.errorToString(code).c_str());
    http.end();
    return false;
}

void sMQTTBroker_User::flushResendQueue()
{
    if (!WiFi.isConnected() || resendQueue.empty())
        return;
    SMQTT_LOGD("Flushing resend queue (%d items)\n", (int)resendQueue.size());
    // Try to send up to N items per loop to avoid long blocking
    size_t toSend = min(resendQueue.size(), (size_t)3);
    for (size_t i = 0; i < toSend; ++i)
    {
        String body = resendQueue.front();
        resendQueue.erase(resendQueue.begin());
        if (!postToBackend(body))
        {
            // put back at end if still failing
            if (resendQueue.size() < MAX_QUEUE)
                resendQueue.push_back(body);
            break;
        }
    }
}
