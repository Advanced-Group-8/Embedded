#include "sMQTTBroker_Modified.h"
#include "auth.h"

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
bool sMQTTBroker_User::onEvent(sMQTTEvent *event)
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

#ifdef ENABLE_BUFFER_LOGGING
    handleMessageBuffer(topic, payload, msgID);
#endif

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    bool isJson = !err;
    if (!isJson)
    {
        SMQTT_LOGD("JSON parse failed: %s\n", err.c_str());
    }
    else
    {
        if (isGpsTopic(topic))
        {
            handleGpsMessage(topic, payload, doc);
        }
        else if (isSensorsTopic(topic))
        {
            handleSensorMessage(topic, payload, doc);
        }
    }
    processQueue();
    return true;
}

bool sMQTTBroker_User::isSensorsTopic(const std::string &topic) const
{
    return topic.rfind("sensors/") == 0;
}

bool sMQTTBroker_User::isGpsTopic(const std::string &topic) const
{
    return topic.rfind("gps/") == 0 || topic.rfind("gps") == 0;
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

void sMQTTBroker_User::handleSensorMessage(const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc)
{
    SMQTT_LOGD("Sensor payload received for topic %s\n", topic.c_str());
    String body;
    body = constructJson(body, topic, payload, doc);
    if (resendQueue.size() >= MAX_QUEUE)
    {
        resendQueue.pop_front();
        SMQTT_LOGD("Resend queue full; dropping oldest to enqueue newest\n");
    }
    resendQueue.push_back(body);
}

String sMQTTBroker_User::constructJson(String &body, const std::string &topic, const std::string &payload, ArduinoJson::JsonDocument &doc)
{
    JsonDocument out;
    out["id"] = 1;
    // out["deviceId"] = WiFi.getHostname() ? WiFi.getHostname() : WiFi.macAddress().c_str();
    // out["deviceId"] = "1337"; // Temporary static ID
    out["deviceId"] = doc["deviceId"].as<String>(); // Temporary static ID
    out["lat"] = lastLat;
    out["lng"] = lastLon;
    out["temperature"] = doc["Temperature"].as<float>();
    out["humidity"] = doc["Humidity"].as<float>();
    out["createdAt"] = currentIsoTimestamp();
    serializeJson(out, body);
    return body;
}

void sMQTTBroker_User::addGpsDataAndTimestamp(ArduinoJson::JsonDocument &doc) const
{
    SMQTT_LOGD("Appending GPS and Timestamp data to JSON\n");
    doc["lat"] = lastLat;
    doc["lng"] = lastLon;
    doc["createdAt"] = currentIsoTimestamp();
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
        SMQTT_LOGD("WiFi not connected; skipping backend POST!\n");
        return false;
    }
    ensureTimeInitialized();

    static bool jwtObtained = false;
    static String jwtToken;
    if (!jwtObtained)
    {
        jwtToken = JWT_signUp();
        if (jwtToken.isEmpty())
        {
            SMQTT_LOGD("Failed to obtain JWT token.\n");
            return false;
        }
        jwtObtained = true;
    }

    String url = String(BACKEND_URL);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(jwtToken));
    SMQTT_LOGD("Posting to backend:\n%s\n", body.c_str());
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

void sMQTTBroker_User::processQueue()
{
    if (!WiFi.isConnected() || resendQueue.empty())
        return;
    SMQTT_LOGD("Processing message queue\n%d item(s) in queue.\n", static_cast<int>(resendQueue.size()));
    size_t toSend = min(resendQueue.size(), (size_t)50); // Limit number of attempts per call - 50 is probably bad!
    for (size_t i = 0; i < toSend; ++i)
    {
        String body = resendQueue.front();
        SMQTT_LOGD("Posting queued message:\n%s\n", body.c_str());
        if (postToBackend(body))
        {
            resendQueue.pop_front();
            SMQTT_LOGD("Post to backend successful!\nCurrently remains: %d item(s) in queue.\n", static_cast<int>(resendQueue.size()));
        }
        else
        {
            SMQTT_LOGD("Post to backend failed!\n%s\nCurrently remains: %d item(s) in queue.\n", body.c_str(), static_cast<int>(resendQueue.size()));
            break;
        }
    }
}

void sMQTTBroker_User::resetGPSCoordinates()
{
    this->lastLat = 0.0;
    this->lastLon = 0.0;
    this->haveGPS = false;
}

#ifdef ENABLE_BUFFER_LOGGING
// Exist for debugging purposes
void sMQTTBroker_User::handleMessageBuffer(const std::string &topic, const std::string &payload, uint16_t msgID)
{
    if (messageBuffer.size() >= MAX_BUFFER_SIZE)
    {
        messageBuffer.pop_front();
    }
    messageBuffer.emplace_back(messageEntry{String(topic.c_str()), String(payload.c_str()), msgID});
    SMQTT_LOGD("New payload buffered:\n%s\n", payload.c_str());
}
#endif

uint16_t sMQTTBroker_User::getClientCount() const
{
    return static_cast<uint16_t>(this->getClients().size());
}
