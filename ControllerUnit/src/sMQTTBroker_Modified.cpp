#include "sMQTTBroker_Modified.h"

/**
 * @brief Processes incoming MQTT events, handling publish events by topic type.
 *
 * For publish events, extracts topic, payload, and message ID, then logs the data.
 * Attempts to parse the payload as JSON; if successful, adds the message ID and dispatches
 * to GPS or sensor handlers based on the topic. If parsing fails, logs the error.
 * Always processes the resend queue to ensure reliable message delivery.
 *
 * @param event Pointer to the received MQTT event.
 * @return true Always returns true to indicate the event was handled.
 */
bool sMQTTBroker_Modified::onEvent(sMQTTEvent *event)
{
    if (event->Type() != Public_sMQTTEventType)
        return true;

    auto *pubEvent = static_cast<sMQTTPublicClientEvent *>(event);
    const std::string &topic = pubEvent->Topic();
    const std::string &payload = pubEvent->Payload();
    const uint16_t msgID = pubEvent->MsgID();

    SMQTT_LOGD("Received topic: %s\n", topic.c_str());
    SMQTT_LOGD("Received Payload:\n%s\n", payload.c_str());
    SMQTT_LOGD("Received Message ID: %u\n", msgID);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    bool isJson = !err;
    if (!isJson)
    {
        SMQTT_LOGD("JSON parse failed: %s\n", err.c_str());
    }
    else
    {
        doc["MessageID"] = msgID;
        if (isGpsTopic(topic))
        {
            handleGpsMessage(topic, payload, doc);
        }
        else if (isSensorsTopic(topic))
        {
            handleSensorMessage(topic, payload, doc);
        }
    }
    return true;
}

bool sMQTTBroker_Modified::isSensorsTopic(const std::string &topic) const
{
    return topic.rfind("sensors/") == 0;
}

bool sMQTTBroker_Modified::isGpsTopic(const std::string &topic) const
{
    return topic.rfind("gps/") == 0 || topic.rfind("gps") == 0;
}

void sMQTTBroker_Modified::handleGpsMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc)
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

void sMQTTBroker_Modified::handleSensorMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc)
{
    SMQTT_LOGD("Sensor payload received for topic %s\n", topic.c_str());
    addGpsDataAndTimestamp(doc);
    String body;
    body = constructJson(body, topic, payload, doc);
    if (messageQueue.size() >= MAX_QUEUE)
    {
        messageQueue.pop_front();
        SMQTT_LOGD("Resend queue full; dropping oldest to enqueue newest\n");
    }
    messageQueue.push_back(body);
}

String sMQTTBroker_Modified::constructJson(String &body, const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc)
{
    JsonDocument out;
    out["Topic"] = topic.c_str();
    out["Controller"] = WiFi.getHostname() ? WiFi.getHostname() : WiFi.macAddress().c_str();
    out["Payload"] = doc;
    serializeJsonPretty(out, body);
    return body;
}

void sMQTTBroker_Modified::addGpsDataAndTimestamp(ArduinoJson::JsonDocument &doc) const
{
    SMQTT_LOGD("Appending ");
    if (haveGPS)
    {
        SMQTT_LOGD("GPS and ");
        doc["GPS"]["Latitude"] = lastLat;
        doc["GPS"]["Longitude"] = lastLon;
    }
    SMQTT_LOGD("Timestamp data to JSON\n");
    doc["Timestamp"] = currentIsoTimestamp();
}

void sMQTTBroker_Modified::ensureTimeInitialized()
{
    if (timeInitialized)
        return;
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    timeInitialized = true;
}

String sMQTTBroker_Modified::currentIsoTimestamp() const
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

bool sMQTTBroker_Modified::postToBackend(const String &body)
{
    if (!WiFi.isConnected())
    {
        SMQTT_LOGD("WiFi not connected; skipping backend POST!\n");
        return false;
    }
    HTTPClient http;
    ensureTimeInitialized();
    String url = String(BACKEND_URL);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(AZURE_JWT_TOKEN));
    SMQTT_LOGD("Posting queued message:\n%s\n", body.c_str());
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

void sMQTTBroker_Modified::processQueue()
{
    if (!WiFi.isConnected() || messageQueue.empty())
        return;
    SMQTT_LOGD("Processing message queue. %d item(s) in queue.\n", static_cast<int>(messageQueue.size()));
    // Limit number of attempts per call - Considering connection drops, having a larger number here means more of the buffer empties on each send.
    // In effect - The broker handles 15+ clients only when this number is unreasonably high.
    size_t toSend = min(messageQueue.size(), (size_t)25);
    for (size_t i = 0; i < toSend; ++i)
    {
        String body = messageQueue.front();
        if (postToBackend(body))
        {
            messageQueue.pop_front();
            SMQTT_LOGD("Post to backend successful!\nCurrently remains: %d item(s) in queue.\n", static_cast<int>(messageQueue.size()));
        }
        else
        {
            SMQTT_LOGD("Post to backend failed!\n%s\nCurrently remains: %d item(s) in queue.\n", body.c_str(), static_cast<int>(messageQueue.size()));
            break;
        }
    }
}

void sMQTTBroker_Modified::resetGPSCoordinates()
{
    this->lastLat = 0.0;
    this->lastLon = 0.0;
    this->haveGPS = false;
}

uint16_t sMQTTBroker_Modified::getClientCount() const
{
    return static_cast<uint16_t>(this->getClients().size());
}
