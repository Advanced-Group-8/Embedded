// THIS IS EXAMPLE CODE. FOLLOWING THIS STRUCTURE: sMQTTBroker_User.h : sMQTTBroker_User.cpp : main.cpp
/*
#include <sMQTTBroker.h>
#include "sMQTTEvent.h"
#include <Arduino.h>

class sMQTTBroker_User : public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event) override;
};

#include "sMQTTBroker_User.h"
#include "sMQTTEvent.h"

bool sMQTTBroker_User::onEvent(sMQTTEvent *event)
{
    if (event->Type() == Public_sMQTTEventType)
    {
        auto *pubEvent = static_cast<sMQTTPublicClientEvent *>(event);
        Serial.print("Received topic: ");
        Serial.println(pubEvent->Topic().c_str());
        Serial.print("Payload: ");
        Serial.println(pubEvent->Payload().c_str());
        // Forward pubEvent->Payload() to your backend here
    }
    return true;
}

#include <Arduino.h>
#include <WiFi.h>
#include "sMQTTBroker_User.h"
#include "arduino_secrets.h"

sMQTTBroker_User broker;

void setup()
{
    Serial.begin(115200);
    delay(5000);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
    }
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    broker.init(MQTT_PORT);
}

void loop()
{
    broker.update();
    delay(1000);
}
*/
