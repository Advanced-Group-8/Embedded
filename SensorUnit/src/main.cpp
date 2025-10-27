#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <PubSubClientQoS2.h>
#include "DHT11.h"
#include "arduino_secrets_template.h"
#include "device_info.h"
#include "eeprom_logging.h"

constexpr bool debugOn = true;
constexpr int DHT11_PIN = 4;
constexpr unsigned long publishInterval = 5000;

DHT11 dht11(DHT11_PIN);
WiFiClient client;
PubSubClient mqttClient(client);

void connectWiFi();
void setupMQTTClient();
void connectMQTT();
void createSensorData(StaticJsonDocument<128> &doc, float temperature, float humidity, const char *deviceID);

static void flushEepromQueue();
static void sendOrEnqueue(const char *payload);
static void buildCompactJson(char out[Elog::RECORD_SIZE],
                             float temperature,
                             float humidity,
                             const char *timestamp,
                             const char *deviceId);

void setup()
{
    Serial.begin(115200);
    dht11.begin();
    connectWiFi();
    initDeviceInfo();
    setupMQTTClient();

    // EEPROM-setup
    bool recovered = Elog::begin();
    if (debugOn)
    {
        Serial.print(F("[Elog] begin(): recovered = "));
        Serial.println(recovered ? "true" : "false");
    }
    Serial.println("Finnished setting up");
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugOn)
            Serial.println(F("[Loop] WiFi not connected, reconnecting..."));
        connectWiFi();
        delay(500);
    }

    if (!mqttClient.connected())
    {
        if (debugOn)
            Serial.println(F("[Loop] MQTT not connected, reconnecting..."));
        connectMQTT();
        //delay(5000);
    }
    mqttClient.loop();

    if(mqttClient.connected())
    {
        flushEepromQueue();
    }

    static unsigned long lastPublish = 0;
    if (millis() - lastPublish >= publishInterval)
    {
        StaticJsonDocument<128> doc;
        float temperature = dht11.getTemperature();
        float humidity = dht11.getHumidity();

        createSensorData(doc, temperature, humidity, getDeviceID());

        char payload[Elog::RECORD_SIZE];
        size_t n = serializeJson(doc, payload, sizeof(payload));
        payload[n < sizeof(payload) ? n : sizeof(payload) - 1] = '\0';

        sendOrEnqueue(payload);
        
        lastPublish = millis();
    }
    // Testing
    static bool dumping = false;
    static unsigned long lastRead = 0;
    if (debugOn && millis() - lastRead > 5000 && dumping == false)
    {
        dumping = true;
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
        const int SHOW = 3;
        for (int i = 0; i < SHOW; i++)
        {
            uint16_t idx = (uint16_t)((head + i) % Elog::CAPACITY);
            Elog::readFromEeprom(idx, record);

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
        dumping = false;
        lastRead = millis();
    }
    delay(500);
}

void setupMQTTClient()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(60);
    mqttClient.setBufferSize(256);
}

void connectWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        uint8_t retry = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print(F("Could not connect to WIFI, retrying..."));
            delay(50);
            retry++;
            if (retry > 40)
            {
                delay(50);
                NVIC_SystemReset();
            }
        }
    }

    if (debugOn && WiFi.status() == WL_CONNECTED)
    {
        Serial.println(F("\nWiFi connected!"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
    }
}

void connectMQTT()
{
    if(WiFi.status() != WL_CONNECTED)
    {
        return;
    }

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
                    snprintf(buf, sizeof(buf), "Connecting to %s:%d", MQTT_BROKER, MQTT_PORT);
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
    doc["DeviceID"] = deviceID;
}

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
        const QOS qos = QOS1; // Default to QoS1
        const bool ok = mqttClient.publish(getMqttTopic(), payload, qos);

        if (ok)
        {
            sent = true;
            if (debugOn)
            {
                Serial.print(F("Published to "));
                Serial.print(getMqttTopic());
                Serial.print(F(" with QoS "));
                Serial.println(qos);
                Serial.println(F("Payload:"));
                Serial.println(payload);
            }
        }

        else if (debugOn)
        {
            Serial.print(F("Publish failed. MQTT state: "));
            Serial.println(mqttClient.state());
        }
    }

    if (!sent)
    {
        // Not online or publish failed -> add to queue
        if (!Elog::enqueuePayload(payload))
        {
            Serial.println(F("[Elog] enqueuePayload FAILED (JSON too long or other issues)."));
        }
        else
        {
            if (debugOn)
                Serial.println(F("[Elog] Enqueued payload (offline/publish fail)."));
        }
    }
}
