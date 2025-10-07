#include "sMQTTBroker_User.h"

bool sMQTTBroker_User::onEvent(sMQTTEvent *event)
{
    if (event->Type() != Public_sMQTTEventType)
        return true;

    auto *pubEvent = static_cast<sMQTTPublicClientEvent *>(event);
    const std::string &topic = pubEvent->Topic();
    const std::string &payload = pubEvent->Payload();
    uint16_t msgID = pubEvent->MsgID();

    SMQTT_LOGD("Received topic: %s\n", topic.c_str());
    SMQTT_LOGD("Payload: %s\n", payload.c_str());
    SMQTT_LOGD("Message ID: %u\n", msgID);
    handleMessageBuffer(topic, payload, msgID);

    // Parse JSON if applicable
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    bool isJson = !err;
    if (!isJson)
    {
        SMQTT_LOGD("JSON parse failed: %s\n", err.c_str());
    }
    else // Check type and dispatch
    {
        if (isGpsTopic(topic))
        {
            handleGpsMessage(topic, payload, doc);
        }
        else if (isSensorsTopic(topic))
        {
            handleSensorMessage(topic, payload, msgID, doc);
        }
    }
    flushResendQueue();
    return true;
}

bool sMQTTBroker_User::isSensorsTopic(const std::string &topic) const
{
    return topic.rfind("sensors/") == 0;
}

bool sMQTTBroker_User::isGpsTopic(const std::string &topic) const
{
    return topic.rfind("owntracks/") == 0;
}

void sMQTTBroker_User::handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc)
{
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

void sMQTTBroker_User::handleSensorMessage(const std::string &topic, const std::string &payload, uint16_t msgID, ArduinoJson::JsonDocument &doc)
{
    SMQTT_LOGD("Sensor payload received for topic %s\n", topic.c_str());
    String body;
    addGpsDataAndTimestamp(doc);
    SMQTT_LOGD("Appending GPS data and timestamp\n");
    body = constructJson(body, topic, payload, msgID, doc);

    if (!postToBackend(body))
    {
        if (resendQueue.size() < MAX_QUEUE)
            resendQueue.push_back(body);
    }
}

String sMQTTBroker_User::constructJson(String &body, const std::string &topic, const std::string &payload, uint16_t msgID, ArduinoJson::JsonDocument &doc)
{
    JsonDocument out;
    out["topic"] = topic.c_str();
    out["controller"] = WiFi.getHostname() ? WiFi.getHostname() : WiFi.macAddress().c_str();
    doc["message_id"] = msgID;
    out["payload"] = doc;
    serializeJsonPretty(out, body);
    return body;
}

void sMQTTBroker_User::addGpsDataAndTimestamp(ArduinoJson::JsonDocument &doc) const
{
    if (haveGPS)
    {
        doc["gps"]["latitude"] = lastLat;
        doc["gps"]["longitude"] = lastLon;
    }
    doc["timestamp"] = currentIsoTimestamp();
}

void sMQTTBroker_User::ensureTimeInitialized()
{
    if (timeInitialized)
        return;
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    timeInitialized = true;
}

String sMQTTBroker_User::currentIsoTimestamp() const
{
    time_t now;
    time(&now);
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
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
    SMQTT_LOGD("Attempting to resend queue (%d items in total).\n", static_cast<int>(resendQueue.size()));
    size_t toSend = min(resendQueue.size(), (size_t)3);
    for (size_t i = 0; i < toSend; ++i)
    {
        String body = resendQueue.front();
        if (postToBackend(body))
        {
            SMQTT_LOGD("Resend successful! Removing:\n'%s'\n from queue.\n", resendQueue.begin()->c_str());
            resendQueue.erase(resendQueue.begin());
        }
    }
}

void sMQTTBroker_User::handleMessageBuffer(const std::string &topic, const std::string &payload, uint16_t msgID)
{
    messageBuffer.push_back({String(topic.c_str()), String(payload.c_str()), msgID});
    if (messageBuffer.size() > MAX_BUFFER_SIZE)
    {
        messageBuffer.erase(messageBuffer.begin());
    }
    SMQTT_LOGD("New payload buffered:\n %s\n", payload.c_str());
}
