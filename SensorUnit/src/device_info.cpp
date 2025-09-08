#include <WiFiS3.h>
#include <stdio.h>
#include "device_info.h"

static char deviceID[18] = "";
static char mqttTopic[64] = "";

void initDeviceInfo()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(deviceID, sizeof(deviceID), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(mqttTopic, sizeof(mqttTopic), "sensors/%s", deviceID);
}

const char *getDeviceID()
{
    return deviceID;
}

const char *getMqttTopic()
{
    return mqttTopic;
}
