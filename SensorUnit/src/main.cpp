#include <Arduino.h>
#include <WiFiS3.h>
#include <time.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include "DHT11.h"
#include "arduino_secrets.h"
#include "device_info.h"

#define SerialAT Serial1
constexpr bool debugOn = true;
constexpr int DHT11_PIN = 4;
constexpr int GMT_OFFSET_PLUS_2 = 7200;

DHT11 dht11(DHT11_PIN);
WiFiClient client;
PubSubClient mqttClient(client);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", GMT_OFFSET_PLUS_2, 60000);

void setupWiFi();
void setupMQTTClient();
void setupNTP();
void reconnectMQTT();
void getTimestamp(char *buffer, size_t len);
void createSensorData(StaticJsonDocument<128> &doc, float temperature, float humidity, const char *timestamp, const char *deviceID);

void setup()
{
    Serial.begin(115200);
    dht11.begin();
    setupWiFi();
    ntpUDP.begin(2390);
    setupNTP();
    initDeviceInfo();
    setupMQTTClient();
}

void loop()
{
    StaticJsonDocument<128> doc;
    char timestamp[25];
    getTimestamp(timestamp, sizeof(timestamp));

    if (!mqttClient.connected())
    {
        if (debugOn)
            Serial.println(F("[Loop] MQTT not connected, reconnecting..."));
        reconnectMQTT();
    }
    mqttClient.loop();

    float temperature = dht11.getTemperature();
    float humidity = dht11.getHumidity();
    if (debugOn)
    {
        Serial.print(F("[Loop] Temperature: "));
        Serial.println(temperature);
        Serial.print(F("[Loop] Humidity: "));
        Serial.println(humidity);
    }

    createSensorData(doc, temperature, humidity, timestamp, getDeviceID());
    char payload[128];
    serializeJsonPretty(doc, payload);
    if (debugOn)
    {
        Serial.print(F("[Loop] Payload: "));
        Serial.println(payload);
    }

    if (debugOn)
        Serial.println(F("[Loop] Publishing to MQTT..."));
    bool published = mqttClient.publish(getMqttTopic(), payload);
    if (debugOn && published)
    {
        Serial.print(F("Published to "));
        Serial.print(getMqttTopic());
        Serial.print(F(": "));
        Serial.println(payload);
    }
    else if (debugOn && !published)
    {
        Serial.println(F("Publish failed"));
    }

    unsigned long now = millis();
    while (millis() - now < 1000)
    {
        mqttClient.loop();
        delay(10);
    }
    delay(20000);
}

void setupMQTTClient()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(60);
    mqttClient.setBufferSize(128); // 256 default
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

    // While developing for debug connectivity issues
    if (debugOn)
    {
        Serial.println(F("\nWiFi connected!"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
    }
}

void setupNTP()
{
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

void reconnectMQTT()
{
    static unsigned long lastDisconnectTime = 0;
    static uint8_t retryCount = 0;
    if (!mqttClient.connected())
    {
        unsigned long currentTime = millis();
        if (currentTime - lastDisconnectTime >= (5000UL << retryCount))
        {
            if (mqttClient.connect(getDeviceID()))
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

void createSensorData(StaticJsonDocument<128> &doc, float temperature, float humidity, const char *timestamp, const char *deviceID)
{
    doc.clear();
    if (isnan(temperature))
    {
        doc["temperature"] = serialized("null");
    }
    else
    {
        doc["temperature"] = temperature;
    }
    if (isnan(humidity))
    {
        doc["humidity"] = serialized("null");
    }
    else
    {
        doc["humidity"] = humidity;
    }
    doc["timestamp"] = timestamp;
}
