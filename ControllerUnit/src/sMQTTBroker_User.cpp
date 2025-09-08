#include "sMQTTBroker_User.h"

std::string lastReceivedPayload;

bool sMQTTBroker_User::onEvent(sMQTTEvent *event)
{
    if (event->Type() == Public_sMQTTEventType)
    {
        auto *pubEvent = static_cast<sMQTTPublicClientEvent *>(event);
        Serial.print("Received topic: ");
        Serial.println(pubEvent->Topic().c_str());
        Serial.print("Payload: ");
        Serial.println(pubEvent->Payload().c_str());
        // Store payload for later extraction
        strncpy(lastReceivedPayload, pubEvent->Payload().c_str(), sizeof(lastReceivedPayload) - 1);
        lastReceivedPayload[sizeof(lastReceivedPayload) - 1] = '\0';
        // Attach controllerUnit ID to messages
        // Attach GPS Coordinates to messages
        // Forward pubEvent->Payload() to your backend here
    }
    return true;
}
