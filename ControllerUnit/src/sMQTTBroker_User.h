#ifndef SMQTTBROKER_USER_H
#define SMQTTBROKER_USER_H

#include "sMQTTBroker.h"
#include "sMQTTEvent.h"
#include <Arduino.h>

class sMQTTBroker_User : public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event) override;
    const char *getLastPayload() const { return lastReceivedPayload; }

private:
    char lastReceivedPayload[256] = {0};
};

#endif // SMQTTBROKER_USER_H
