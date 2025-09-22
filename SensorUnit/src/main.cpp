#include <Arduino.h>
#include <WiFiS3.h>
#include <time.h>
#include <ArduinoJson.h>
#include <PubSubClientQoS2.h>
#include <NTPClient.h>
#include "DHT11.h"
#include "arduino_secrets_template.h"
#include "device_info.h"
#include "eeprom_logging.h"

constexpr bool debugOn = true;
constexpr int DHT11_PIN = 4;
constexpr int GMT_OFFSET_PLUS_2 = 7200;

DHT11 dht11(DHT11_PIN);
WiFiClient client;
PubSubClient mqttClient(client);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", GMT_OFFSET_PLUS_2, 60000);

void setupWiFi();
void callback(char *topic, uint8_t *payload, unsigned int length);
void setupMQTTClient();
void setupNTP();
void connectMQTT();
void getTimestamp(char *buffer, size_t len);
void createSensorData(StaticJsonDocument<256> &doc, float temperature, float humidity, const char *timestamp, const char *deviceID);

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
    Serial.println("Starting up...");
    dht11.begin();
    setupWiFi();
    setupNTP();
    initDeviceInfo();
    setupMQTTClient();

    //EEPROM-setup
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
    if (!mqttClient.connected())
    {
        if (debugOn) 
        {
            Serial.println(F("[Loop] MQTT not connected, reconnecting..."));
        }
        connectMQTT();
    }
    mqttClient.loop();

    if(mqttClient.connected())
    {
        flushEepromQueue();
    }

    static unsigned long lastPublish = 0;
    const unsigned long publishInterval = 5000;
    if (millis() - lastPublish >= publishInterval)
    {
        char timestamp[25];
        getTimestamp(timestamp, sizeof(timestamp));

        float temperature = dht11.getTemperature();
        float humidity    = dht11.getHumidity();

        char payload[Elog::RECORD_SIZE];
        buildCompactJson(payload, temperature, humidity, timestamp, getDeviceID());

        sendOrEnqueue(payload);

        lastPublish = millis();
    }
    delay(100);

    //Testing
    static unsigned long lastRead = 0;
    if (debugOn && millis() - lastRead > 5000)
    {
        Serial.println(F("[Elog] Current Indexes:"));
        Serial.print(F("  ReadIndex: "));  Serial.println(Elog::getReadIndex());
        Serial.print(F("  WriteIndex: ")); Serial.println(Elog::getWriteIndex());
        Serial.print(F("  Count: "));      Serial.println(Elog::getQueueCount());

        Serial.println();

        char record[Elog::RECORD_SIZE];

        uint16_t head = Elog::getReadIndex();
        for (int i = 0; i < 9; i++)
        {
            Elog::readFromEeprom(i, record);

            uint8_t  status = (uint8_t)record[Elog::REC_STATUS_OFF];
            uint16_t len    = (uint16_t)( (uint8_t)record[Elog::REC_LEN_OFF + 0]
                                        | ((uint16_t)(uint8_t)record[Elog::REC_LEN_OFF + 1] << 8) );

            Serial.print(F("#")); Serial.println(i + 1);
            Serial.print(F("  status=0x")); Serial.println(status, HEX);
            Serial.print(F("  len="));      Serial.println(len);

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
}

void setupMQTTClient()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(60);
    mqttClient.setBufferSize(256);
}

void setupWiFi()
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

    if (debugOn)
    {
        Serial.println(F("\nWiFi connected!"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
    }
}

void setupNTP()
{
    ntpUDP.begin(2390);
    timeClient.begin();
    while (!timeClient.update())
    {
        delay(100);
    }
}

void getTimestamp(char *buffer, size_t len)
{
    if (!timeClient.update())
    {
        timeClient.forceUpdate();
    }
    time_t rawTime = timeClient.getEpochTime();
    struct tm timeinfo;
    struct tm *tmptr = gmtime(&rawTime);
    if (tmptr)
    {
        timeinfo = *tmptr;
    }
    snprintf(buffer, len, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
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

void createSensorData(StaticJsonDocument<256> &doc, float temperature, float humidity, const char *timestamp, const char *deviceID)
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
    doc["Timestamp"] = timestamp;
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
  if (isnan(temperature)) doc["t"] = serialized("null"); else doc["t"] = temperature;
  if (isnan(humidity))    doc["h"] = serialized("null"); else doc["h"] = humidity;
  doc["ts"]  = timestamp;
  doc["dev"] = deviceId;
  doc["seq"] = Elog::getAndIncrementSequence();

  size_t n = serializeJson(doc, out, Elog::MAXJSON_CHARS + 1);
  out[n] = '\0';
}

static void flushEepromQueue()
{
    if (!mqttClient.connected()) return;

    char payload[Elog::RECORD_SIZE];
    while (Elog::hasPending())
    {    
        if (!Elog::peekPending(payload)) break;

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
            Serial.println(F("[Elog] enqueuePayload FAILED (för långt JSON eller annat fel)."));
        }
        else
        {
            if (debugOn) Serial.println(F("[Elog] Enqueued payload (offline/publish fail)."));
        }
    }
}