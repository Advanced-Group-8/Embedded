#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <PubSubClientQoS2.h>
#include "DHT11.h"
#include "arduino_secrets.h"
#include "device_info.h"
#ifdef EEPROM_SUPPORT
#include "eeprom_logging.h"
#endif

#ifdef DEBUG_ON
constexpr bool debugOn = true;
#else
constexpr bool debugOn = false;
#endif
constexpr int DHT11_PIN = 4;
constexpr unsigned long publishInterval = 5000;

DHT11 dht11(DHT11_PIN);
WiFiClient client;
PubSubClient mqttClient(client);

void connectWiFi();
void setupMQTTClient();
void connectMQTT();
void createSensorData(StaticJsonDocument<128> &doc, float temperature, float humidity, const char *deviceID);

#ifdef EEPROM_SUPPORT
static void flushEepromQueue();
static void sendOrEnqueue(const char *payload);
static void buildCompactJson(char out[Elog::RECORD_SIZE], float temperature, float humidity, const char *timestamp, const char *deviceId);
#endif

void setup()
{
    Serial.begin(115200);
    dht11.begin();
    connectWiFi();
    initDeviceInfo();
    setupMQTTClient();

#ifdef EEPROM_SUPPORT
    // EEPROM-setup
    bool recovered = Elog::begin();
    if (debugOn)
    {
        Serial.print(F("[Elog] begin(): recovered = "));
        Serial.println(recovered ? "true" : "false");
    }
#endif
    Serial.println("Finished setup");
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED || !mqttClient.connected())
    {
        if (WiFi.status() != WL_CONNECTED)
            connectWiFi();
        if (!mqttClient.connected())
            connectMQTT();
    }
    mqttClient.loop();

    static unsigned long lastPublish = 0;
    if (millis() - lastPublish >= publishInterval)
    {
        StaticJsonDocument<128> doc;
        float temperature = dht11.getTemperature();
        float humidity = dht11.getHumidity();

        createSensorData(doc, temperature, humidity, getDeviceID());
        char payload[128];
#ifdef DEBUG_ON
        serializeJsonPretty(doc, payload);
#else
        serializeJson(doc, payload);
#endif
        if (mqttClient.publish(getMqttTopic(), payload, QOS1))
        {
            if (debugOn)
            {
                Serial.print(F("Published to "));
                Serial.print(getMqttTopic());
                Serial.print(F(" with QoS 1: \n"));
                Serial.println(payload);
            }
            else
            {
                if (debugOn)
                {
                    Serial.println(F("Publish failed with QoS 1"));
                    Serial.print(F("MQTT state: "));
                    Serial.println(mqttClient.state());
                }
            }
        }
        lastPublish = millis();
    }

#ifdef EEPROM_SUPPORT
    // Testing
    static unsigned long lastRead = 0;
    if (debugOn && millis() - lastRead > 5000)
    {
        Serial.println(F("[Elog] Current Indexes:"));
        Serial.print(F("  ReadIndex: "));
        Serial.println(Elog::getReadIndex());
        Serial.print(F("  WriteIndex: "));
        Serial.println(Elog::getWriteIndex());
        Serial.print(F("  Count: "));
        Serial.println(Elog::getQueueCount());

        Serial.println();

        char record[Elog::RECORD_SIZE];

        uint16_t head = Elog::getReadIndex();
        for (int i = 0; i < 9; i++)
        {
            Elog::readFromEeprom(i, record);

            uint8_t status = (uint8_t)record[Elog::REC_STATUS_OFF];
            uint16_t len = (uint16_t)((uint8_t)record[Elog::REC_LEN_OFF + 0] | ((uint16_t)(uint8_t)record[Elog::REC_LEN_OFF + 1] << 8));

            Serial.print(F("#"));
            Serial.println(i + 1);
            Serial.print(F("  status=0x"));
            Serial.println(status, HEX);
            Serial.print(F("  len="));
            Serial.println(len);

            Serial.println(F("  payload:"));
            if (status == Elog::STATUS_PENDING || status == Elog::STATUS_SENT)
            {
                Serial.write(&record[Elog::REC_DATA_OFF], len);
                Serial.println();
            }
            else
            {
                Serial.println(F("  <empty>"));
            }
            Serial.println(F("---------------------------------"));
            delay(50);
        }
        Serial.println();
        lastRead = millis();
    }
#endif
    delay(100);
}

void setupMQTTClient()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(60);
    mqttClient.setBufferSize(128);
}

void connectWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        uint8_t retry = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            retry++;
            if (retry > 40)
            {
                delay(2000);
                NVIC_SystemReset();
            }
        }
    }
    else if (debugOn && WiFi.status() == WL_CONNECTED)
    {
        Serial.println(F("\nWiFi connected!"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
    }
}

void connectMQTT()
{
    static unsigned long lastDisconnectTime = 0;
    static uint8_t retryCount = 0;
    if (!mqttClient.connected())
    {
        unsigned long currentTime = millis();
        if (currentTime - lastDisconnectTime >= (5000UL << retryCount))
        {
            {
                if (debugOn)
                {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "[MQTT] Connecting to %s:%d", MQTT_BROKER, MQTT_PORT);
                    Serial.println(buf);
                }
            }
            if (mqttClient.connect(getDeviceID(), false))
            {
                retryCount = 0;
                if (debugOn)
                    Serial.println(F("[MQTT] Connected!"));
            }
            else
            {
                if (debugOn)
                {
                    Serial.print(F("[MQTT] Connect failed, state: "));
                    Serial.println(mqttClient.state());
                }
                retryCount = min(retryCount + 1, 5);
            }
            lastDisconnectTime = currentTime;
        }
    }
}

void createSensorData(StaticJsonDocument<128> &doc, float temperature, float humidity, const char *deviceID)
{
    doc.clear();
    if (isnan(temperature))
    {
        doc["Temperature"] = serialized("null");
    }
    else
    {
        doc["Temperature"] = temperature;
    }
    if (isnan(humidity))
    {
        doc["Humidity"] = serialized("null");
    }
    else
    {
        doc["Humidity"] = humidity;
    }
}

#ifdef EEPROM_SUPPORT
// ----- EEPROM-logging functions -----
static void buildCompactJson(char out[Elog::RECORD_SIZE],
                             float temperature,
                             float humidity,
                             const char *timestamp,
                             const char *deviceId)
{
    // short keys save space
    StaticJsonDocument<160> doc;
    if (isnan(temperature))
        doc["t"] = serialized("null");
    else
        doc["t"] = temperature;

    if (isnan(humidity))
        doc["h"] = serialized("null");
    else
        doc["h"] = humidity;

    doc["ts"] = timestamp;
    doc["dev"] = deviceId;
    doc["seq"] = Elog::getAndIncrementSequence();
    size_t n = serializeJson(doc, out, Elog::MAXJSON_CHARS + 1);
    out[n] = '\0';
}

static void flushEepromQueue()
{
    if (!mqttClient.connected())
        return;

    char payload[Elog::RECORD_SIZE];
    while (Elog::hasPending())
    {
        if (!Elog::peekPending(payload))
            break;

        // Using QoS1 as default. Can be changed to QoS2 if desired
        if (mqttClient.publish(getMqttTopic(), payload, QOS1))
        {
            Elog::markCurrentAsSent();
            if (debugOn)
            {
                Serial.println(F("[Elog] Flushed one pending payload."));
            }
        }
        else
        {
            if (debugOn)
            {
                Serial.print(F("[Elog] Flush publish failed. MQTT state: "));
                Serial.println(mqttClient.state());
            }
            // Abort flush now. We try again next loop when connection is stable
            break;
        }
    }
}

static void sendOrEnqueue(const char *payload)
{
    bool sent = false;

    if (mqttClient.connected())
    {
        // Try from QoS0 up to QoS2, break at first successful publish
        for (uint8_t qos = QOS0; qos <= QOS2; ++qos)
        {
            if (mqttClient.publish(getMqttTopic(), payload, static_cast<QOS>(qos)))
            {
                sent = true;
                if (debugOn)
                {
                    Serial.print(F("Published to "));
                    Serial.print(getMqttTopic());
                    Serial.print(F(" with QoS "));
                    Serial.println(qos);
                }
                break;
            }
        }

        if (!sent && debugOn)
        {
            Serial.print(F("Publish failed at all QoS levels. MQTT state: "));
            Serial.println(mqttClient.state());
        }
    }

    if (!sent)
    {
        // Not online or publish failed -> add to queue
        if (!Elog::enqueuePayload(payload))
        {
            Serial.println(F("[Elog] enqueuePayload FAILED (JSON too big or other issues)."));
        }
        else
        {
            if (debugOn)
                Serial.println(F("[Elog] Enqueued payload (offline/publish fail)."));
        }
    }
}
#endif
