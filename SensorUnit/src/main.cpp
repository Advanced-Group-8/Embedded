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

void setup()
{
    Serial.begin(115200);
    dht11.begin();
    //setupWiFi();
    //setupNTP();
    //initDeviceInfo();
    //setupMQTTClient();

    randomSeed(analogRead(A0));
    delay(10000);
}

//First test-loop
void loop()
{
    uint8_t randomTemperature = random(0, 31);
    delay(100);
    uint8_t randomHumidity = random(9, 101);

    // First I start the program with "testStep = 0". After I see everything works as intended I cut the power for the device,
    // and declare "testStep = 3" before I run the program again to ensure the previus data is still stored on the EEPROM
    static uint8_t testStep = 3; 

    char payloadBuffer[Elog::RECORD_SIZE]{};
    char readBuffer[Elog::RECORD_SIZE];

    if(testStep == 0)
    {
        StaticJsonDocument<256> doc;
        char timeStamp[25];
        snprintf(timeStamp, sizeof timeStamp, "TEST-%lu", millis());
        createSensorData(doc, randomTemperature, randomHumidity, timeStamp, "DEV");

        uint32_t sequence = Elog::getAndIncrementSequence();
        doc["seq"] = sequence;
        doc["buffered"] = true;
        serializeJson(doc, payloadBuffer, Elog::MAXJSON_CHARS);

        Elog::enqueuePayload(payloadBuffer);
        uint16_t newest = (Elog::getWriteIndex() + Elog::CAPACITY - 1) % Elog::CAPACITY;
        delay(1000);

        Serial.println("----------------------------------");
        Serial.print(F("[#1] wrote slot ")); Serial.println(newest);
        Serial.print(F("Read Index = ")); Serial.print(Elog::getReadIndex());
        Serial.print(F("Write Index = ")); Serial.print(Elog::getWriteIndex());
        Serial.print(F("Count Index = ")); Serial.println(Elog::getQueueCount());
        Serial.println("----------------------------------");

        delay(1000);
        testStep = 1;
    }
    else if(testStep == 1)
    {
        StaticJsonDocument<256> doc;
        char timeStamp[25];
        snprintf(timeStamp, sizeof timeStamp, "TEST-%lu", millis());
        createSensorData(doc, randomTemperature, randomHumidity, timeStamp, "DEV");

        uint32_t sequence = Elog::getAndIncrementSequence();
        doc["seq"] = sequence;
        doc["buffered"] = true;
        serializeJson(doc, payloadBuffer, Elog::MAXJSON_CHARS);

        Elog::enqueuePayload(payloadBuffer);
        uint16_t newest = (Elog::getWriteIndex() + Elog::CAPACITY - 1) % Elog::CAPACITY;
        uint16_t prev1  = (Elog::getWriteIndex() + Elog::CAPACITY - 2) % Elog::CAPACITY;
        delay(1000);

        Serial.println();
        Serial.print(F("[#2] wrote slot ")); Serial.println(newest);
        Serial.println("----------------------------------");

        Elog::readFromEeprom(newest, readBuffer);
        Serial.print(F("   This slot ")); Serial.print(newest); Serial.print(F(": "));
        Serial.println(readBuffer);
        Serial.println("----------------------------------");
        Serial.println();

        Elog::readFromEeprom(prev1, readBuffer);
        Serial.print(F("   previous slot ")); Serial.print(prev1); Serial.print(F(": "));
        Serial.println(readBuffer);
        Serial.println("----------------------------------");
        Serial.println();

        delay(1000);
        testStep = 2;
    }
    else if(testStep == 2)
    {
        StaticJsonDocument<256> doc;
        char timeStamp[25];
        snprintf(timeStamp, sizeof timeStamp, "TEST-%lu", millis());
        createSensorData(doc, randomTemperature, randomHumidity, timeStamp, "DEV");

        uint32_t sequence = Elog::getAndIncrementSequence();
        doc["seq"] = sequence;
        doc["buffered"] = true;
        serializeJson(doc, payloadBuffer, Elog::MAXJSON_CHARS);

        Elog::enqueuePayload(payloadBuffer);
        uint16_t newest = (Elog::getWriteIndex() + Elog::CAPACITY - 1) % Elog::CAPACITY;
        uint16_t prev1  = (Elog::getWriteIndex() + Elog::CAPACITY - 2) % Elog::CAPACITY;
        uint16_t prev2  = (Elog::getWriteIndex() + Elog::CAPACITY - 3) % Elog::CAPACITY;
        delay(1000);

        Serial.println();
        Serial.print(F("[#3] wrote slot ")); Serial.println(newest);
        Serial.println("----------------------------------");
        
        Elog::readFromEeprom(newest, readBuffer);
        Serial.print(F("   This slot ")); Serial.print(newest); Serial.print(F(": "));
        Serial.println(readBuffer);
        Serial.println("----------------------------------");

        Elog::readFromEeprom(prev1, readBuffer);
        Serial.print(F("   previous slot ")); Serial.print(prev1); Serial.print(F(": "));
        Serial.println(readBuffer);
        Serial.println("----------------------------------");

        Elog::readFromEeprom(prev2, readBuffer);
        Serial.print(F("   previous slot ")); Serial.print(prev2); Serial.print(F(": "));
        Serial.println(readBuffer);

        Serial.println("----------------------------------");
        Serial.print(F("Read Index = ")); Serial.print(Elog::getReadIndex());
        Serial.print(F("Write Index = ")); Serial.print(Elog::getWriteIndex());
        Serial.print(F("Count Index = ")); Serial.println(Elog::getQueueCount());
        Serial.println("----------------------------------");
        Serial.println();

        delay(1000);
        testStep = 3;
    }
    else if (testStep == 3)
    {
        //Read ring-buffer metadata from EEPROM (little-endian u16 values).
        uint16_t writeIndexE = (uint16_t)EEPROM.read(Elog::WRITE_INDEX_ADDRESS)
                            | (uint16_t)(EEPROM.read(Elog::WRITE_INDEX_ADDRESS + 1) << 8);
        uint16_t countE      = (uint16_t)EEPROM.read(Elog::COUNT_ADDRESS)
                            | (uint16_t)(EEPROM.read(Elog::COUNT_ADDRESS + 1) << 8);

        uint16_t howMany = (countE < 3) ? countE : 3;
        if (howMany == 0) {
            Serial.println(F("[dump] queue is empty"));
            delay(2000);
            return;
        }

        // Decide how many records to show on boot.
        // Show up to 3, but never more than what is currently stored in the queue.
        uint16_t start = (writeIndexE + Elog::CAPACITY - howMany) % Elog::CAPACITY;

        for (uint16_t k = 0; k < howMany; ++k)
        {
            uint16_t idx = (start + k) % Elog::CAPACITY;
            Elog::readFromEeprom(idx, readBuffer);
            Serial.print(F("[dump] slot ")); Serial.print(idx); Serial.print(F(": "));
            Serial.println(readBuffer);
            Serial.println("----------------------------------");
        }
        Serial.println(F("Test complete"));
        delay(2000);
    }
}

/*void loop()
{
    if (!mqttClient.connected())
    {
        if (debugOn)
            Serial.println(F("[Loop] MQTT not connected, reconnecting..."));
        connectMQTT();
        delay(5000);
    }
    mqttClient.loop();

    static unsigned long lastPublish = 0;
    const unsigned long publishInterval = 15000;
    if (millis() - lastPublish >= publishInterval)
    {
        StaticJsonDocument<256> doc;
        char timestamp[25];
        getTimestamp(timestamp, sizeof(timestamp));
        float temperature = dht11.getTemperature();
        float humidity = dht11.getHumidity();

        createSensorData(doc, temperature, humidity, timestamp, getDeviceID());
        char payload[256];
        serializeJsonPretty(doc, payload);
        for (uint8_t qos = QOS0; qos <= QOS2; ++qos)
        {
            if (mqttClient.publish(getMqttTopic(), payload, static_cast<QOS>(qos)))
            {
                if (debugOn)
                {
                    Serial.print(F("Published to "));
                    Serial.print(getMqttTopic());
                    Serial.print(F(" with QoS "));
                    Serial.print(qos);
                    Serial.print(F(": \n"));
                    Serial.println(payload);
                }
            }
            else
            {
                if (debugOn)
                {
                    Serial.print(F("Publish failed with QoS "));
                    Serial.print(qos);
                    Serial.println();
                    Serial.print(F("MQTT state: "));
                    Serial.println(mqttClient.state());
                }
            }
        }
        lastPublish = millis();
    }
    delay(100);
}*/

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
