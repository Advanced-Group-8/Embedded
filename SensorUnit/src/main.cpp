#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <PubSubClientQoS2.h>
#include "DHT11.h"
#include "arduino_secrets.h"
#include "device_info.h"

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

void setup()
{
    Serial.begin(115200);
    dht11.begin();
    connectWiFi();
    initDeviceInfo();
    setupMQTTClient();
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
    }

    if (!mqttClient.connected())
    {
        if (debugOn)
            Serial.println(F("[Loop] MQTT not connected, reconnecting..."));
        connectMQTT();
        delay(5000);
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
        serializeJsonPretty(doc, payload);
        for (uint8_t qos = QOS1; qos <= QOS1; ++qos)
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
    delay(500);
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

    if (debugOn && WiFi.status() == WL_CONNECTED)
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
}
