/*
  PubSubClientQoS2.cpp - A simple client for MQTT.
  Based on work by Nick O'Leary
  Modified by Oscar A
*/

#ifndef PubSubClientQoS2_h
#define PubSubClientQoS2_h

#include <Arduino.h>
#include "IPAddress.h"
#include "Client.h"
#include "Stream.h"

#define MQTT_VERSION_3_1 3
#define MQTT_VERSION_3_1_1 4

// MQTT_VERSION : Pick the version
// #define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size. Override with setBufferSize().
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 128
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds. Override with setKeepAlive()
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 15
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override with setSocketTimeout()
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

// MQTT_MAX_TRANSFER_SIZE : limit how much data is passed to the network client
// in each write call. Needed for the Arduino Wifi Shield. Leave undefined to
// pass the entire MQTT packet in each write call.
// #define MQTT_MAX_TRANSFER_SIZE 80

// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT -4
#define MQTT_CONNECTION_LOST -3
#define MQTT_CONNECT_FAILED -2
#define MQTT_DISCONNECTED -1
#define MQTT_CONNECTED 0
#define MQTT_CONNECT_BAD_PROTOCOL 1
#define MQTT_CONNECT_BAD_CLIENT_ID 2
#define MQTT_CONNECT_UNAVAILABLE 3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED 5

#define MQTTCONNECT 1 << 4      // Client request to connect to Server
#define MQTTCONNACK 2 << 4      // Connect Acknowledgment
#define MQTTPUBLISH 3 << 4      // Publish message
#define MQTTPUBACK 4 << 4       // Publish Acknowledgment
#define MQTTPUBREC 5 << 4       // Publish Received (assured delivery part 1)
#define MQTTPUBREL 6 << 4       // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP 7 << 4      // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE 8 << 4    // Client Subscribe request
#define MQTTSUBACK 9 << 4       // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK 11 << 4    // Unsubscribe Acknowledgment
#define MQTTPINGREQ 12 << 4     // PING Request
#define MQTTPINGRESP 13 << 4    // PING Response
#define MQTTDISCONNECT 14 << 4  // Client is Disconnecting
#define MQTTReserved 15 << 4    // Reserved

#define MQTTQOS0 (0 << 1)
#define MQTTQOS1 (1 << 1)
#define MQTTQOS2 (2 << 1)

// Maximum size of fixed header and variable length size header
#define MQTT_MAX_HEADER_SIZE 5

// QoS2 Support

// QOS Support: Adjust downwards to save memory drastically
#ifndef MAX_TOPIC_LENGTH
#define MAX_TOPIC_LENGTH 256
#endif
#ifndef MAX_PAYLOAD_LENGTH
#define MAX_PAYLOAD_LENGTH 256
#endif
#ifndef MAX_OUTGOING_QOS2_MESSAGES
#define MAX_OUTGOING_QOS2_MESSAGES 10
#endif
#ifndef MAX_INCOMING_QOS2_MESSAGES
#define MAX_INCOMING_QOS2_MESSAGES 10
#endif

enum QOS : uint8_t
{
    QOS0 = 0,
    QOS1,
    QOS2
};

enum OutgoingQoS2State
{
    Q2_DONE = 0,
    Q2_WAIT_PUBREC,
    Q2_WAIT_PUBCOMP
};

enum IncomingQoS2State
{
    Q2_IDLE = 0,
    Q2_RECEIVED_PUBLISH
};

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define MQTT_CALLBACK_SIGNATURE std::function<void(char *, uint8_t *, unsigned int)> callback
#else
#define MQTT_CALLBACK_SIGNATURE void (*callback)(char *, uint8_t *, unsigned int)
#endif

#define CHECK_STRING_LENGTH(l, s)                                \
    if (l + 2 + strnlen(s, this->bufferSize) > this->bufferSize) \
    {                                                            \
        _client->stop();                                         \
        return false;                                            \
    }

class PubSubClient : public Print
{
private:
    Client *_client;
    uint8_t *buffer;
    uint16_t bufferSize;
    uint16_t keepAlive;
    uint16_t socketTimeout;
    uint16_t nextMsgId;
    unsigned long lastOutActivity;
    unsigned long lastInActivity;
    bool pingOutstanding;
    MQTT_CALLBACK_SIGNATURE;
    uint32_t readPacket(uint8_t *);
    boolean readByte(uint8_t *result);
    boolean readByte(uint8_t *result, uint16_t *index);
    boolean write(uint8_t header, uint8_t *buf, uint16_t length);
    uint16_t writeString(const char *string, uint8_t *buf, uint16_t pos);
    // Build up the header ready to send
    // Returns the size of the header
    // Note: the header is built at the end of the first MQTT_MAX_HEADER_SIZE bytes, so will start
    //       (MQTT_MAX_HEADER_SIZE - <returned size>) bytes into the buffer
    size_t buildHeader(uint8_t header, uint8_t *buf, uint16_t length);
    IPAddress ip;
    const char *domain;
    uint16_t port;
    Stream *stream;
    int _state;

    // QoS2 Support Below This Line
    // Will message parameters
    char willTopic[MAX_TOPIC_LENGTH];
    byte willQos;

    struct OutgoingQoS2Message
    {
        uint16_t msgId = 0;
        OutgoingQoS2State state = Q2_DONE;
        unsigned long timestamp = 0;
        bool dup = false;
        char topic[MAX_TOPIC_LENGTH] = {};
        uint8_t payload[MAX_PAYLOAD_LENGTH] = {};
        size_t payloadLength = 0;
    };

    struct IncomingQoS2Message
    {
        uint16_t msgId = 0;
        IncomingQoS2State state = Q2_IDLE;
        char topic[MAX_TOPIC_LENGTH] = {};
        uint8_t payload[MAX_PAYLOAD_LENGTH] = {};
        size_t payloadLength = 0;
    };

    OutgoingQoS2Message outgoingMessages[MAX_OUTGOING_QOS2_MESSAGES];
    IncomingQoS2Message incomingMessages[MAX_INCOMING_QOS2_MESSAGES];
    void initOutgoingMessages();
    void resendOutgoingMessages();

public:
    PubSubClient();
    explicit PubSubClient(Client &client);
    PubSubClient(IPAddress, uint16_t, Client &client);
    PubSubClient(IPAddress, uint16_t, Client &client, Stream &);
    PubSubClient(IPAddress, uint16_t, MQTT_CALLBACK_SIGNATURE, Client &client);
    PubSubClient(IPAddress, uint16_t, MQTT_CALLBACK_SIGNATURE, Client &client, Stream &);
    PubSubClient(const uint8_t *, uint16_t, Client &client);
    PubSubClient(const uint8_t *, uint16_t, Client &client, Stream &);
    PubSubClient(const uint8_t *, uint16_t, MQTT_CALLBACK_SIGNATURE, Client &client);
    PubSubClient(const uint8_t *, uint16_t, MQTT_CALLBACK_SIGNATURE, Client &client, Stream &);
    PubSubClient(const char *, uint16_t, Client &client);
    PubSubClient(const char *, uint16_t, Client &client, Stream &);
    PubSubClient(const char *, uint16_t, MQTT_CALLBACK_SIGNATURE, Client &client);
    PubSubClient(const char *, uint16_t, MQTT_CALLBACK_SIGNATURE, Client &client, Stream &);

    ~PubSubClient();

    PubSubClient &setServer(IPAddress ip, uint16_t port);
    PubSubClient &setServer(const uint8_t *ip, uint16_t port);
    PubSubClient &setServer(const char *domain, uint16_t port);
    PubSubClient &setCallback(MQTT_CALLBACK_SIGNATURE);
    PubSubClient &setClient(Client &client);
    PubSubClient &setStream(Stream &stream);
    PubSubClient &setKeepAlive(uint16_t keepAlive);
    PubSubClient &setSocketTimeout(uint16_t timeout);

    boolean setBufferSize(uint16_t size);
    uint16_t getBufferSize();

    boolean connect(const char *id);
    boolean connect(const char *id, boolean cleanSession);
    boolean connect(const char *id, const char *user, const char *pass);
    boolean connect(const char *id, const char *user, const char *pass, boolean cleanSession);
    boolean connect(const char *id, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage);
    boolean connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage);
    boolean connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage, boolean cleanSession);
    void disconnect();
    boolean publish(const char *topic, const char *payload);
    boolean publish(const char *topic, const char *payload, boolean retained);
    boolean publish(const char *topic, const uint8_t *payload, unsigned int plength);
    boolean publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained);
    boolean publish(const char *topic, const char *payload, uint8_t qos);                                            // QoS 2 Support publish overload
    boolean publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained, uint8_t qos); // QoS 2 Support publish overload
    boolean publish(const char *topic, const char *payload, boolean retained, uint8_t qos);                          // QoS 2 Support publish overload
    boolean publish(const char *topic, const char *payload, QOS qos);                                                // QoS 2 Support publish overload
    boolean publish_P(const char *topic, const char *payload, boolean retained);
    boolean publish_P(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained);
    // Start to publish a message.
    // This API:
    //   beginPublish(...)
    //   one or more calls to write(...)
    //   endPublish()
    // Allows for arbitrarily large payloads to be sent without them having to be copied into
    // a new buffer and held in memory at one time
    // Returns 1 if the message was started successfully, 0 if there was an error
    boolean beginPublish(const char *topic, unsigned int plength, boolean retained);
    // Finish off this publish message (started with beginPublish)
    // Returns 1 if the packet was sent successfully, 0 if there was an error
    int endPublish();
    // Write a single byte of payload (only to be used with beginPublish/endPublish)
    size_t write(uint8_t) override;
    // Write size bytes from buffer into the payload (only to be used with beginPublish/endPublish)
    // Returns the number of bytes written
    size_t write(const uint8_t *buffer, size_t size) override;
    boolean subscribe(const char *topic);
    boolean subscribe(const char *topic, uint8_t qos);
    boolean unsubscribe(const char *topic);
    boolean loop();
    boolean connected();
    int state();
};

#endif
