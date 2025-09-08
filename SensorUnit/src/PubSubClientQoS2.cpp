/*
  PubSubClientQoS2.cpp - A simple client for MQTT.
  Based on work by Nick O'Leary
  Modified by Oscar A
*/

#include "PubSubClientQoS2.h"
#include "Arduino.h"

PubSubClient::PubSubClient()
{
    this->_state = MQTT_DISCONNECTED;
    this->_client = NULL;
    this->stream = NULL;
    setCallback(NULL);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(addr, port);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client &client, Stream &stream)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(addr, port);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(addr, port);
    setCallback(callback);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client &client, Stream &stream)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(addr, port);
    setCallback(callback);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const uint8_t *ip, uint16_t port, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(ip, port);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const uint8_t *ip, uint16_t port, Client &client, Stream &stream)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(ip, port);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const uint8_t *ip, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(ip, port);
    setCallback(callback);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const uint8_t *ip, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client &client, Stream &stream)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(ip, port);
    setCallback(callback);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const char *domain, uint16_t port, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(domain, port);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const char *domain, uint16_t port, Client &client, Stream &stream)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(domain, port);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const char *domain, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(domain, port);
    setCallback(callback);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::PubSubClient(const char *domain, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client &client, Stream &stream)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(domain, port);
    setCallback(callback);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    initOutgoingMessages(); // QoS2 Support
}

PubSubClient::~PubSubClient()
{
    free(this->buffer);
}

void PubSubClient::initOutgoingMessages()
{
    for (int i = 0; i < MAX_OUTGOING_QOS2_MESSAGES; i++)
    {
        outgoingMessages[i].state = Q2_DONE;
        outgoingMessages[i].msgId = 0;
        outgoingMessages[i].payloadLength = 0;

        // Clear topic buffer
        memset(outgoingMessages[i].topic, 0, sizeof(outgoingMessages[i].topic));

        // Clear payload buffer
        memset(outgoingMessages[i].payload, 0, sizeof(outgoingMessages[i].payload));

        outgoingMessages[i].dup = false;
        outgoingMessages[i].timestamp = 0;
    }
}

void PubSubClient::resendOutgoingMessages()
{
    // Called on reconnect if cleanSession == false.
    // Must resend PUBLISH (with DUP=1) if waiting PUBREC, and must resend PUBREL if waiting PUBCOMP.
    for (int i = 0; i < MAX_OUTGOING_QOS2_MESSAGES; i++)
    {
        if (outgoingMessages[i].msgId == 0)
        {
            continue;
        }

        if (outgoingMessages[i].state == Q2_WAIT_PUBREC)
        {
            // Resend the PUBLISH with DUP flag set (set dup BEFORE building header so header includes DUP)
            uint8_t header = MQTTPUBLISH | MQTTQOS2;
            // ensure DUP is set for retransmit
            outgoingMessages[i].dup = true;
            header |= 0x08; // DUP bit set

            // Prepare the buffer with topic and payload
            uint16_t topicLen = strnlen(outgoingMessages[i].topic, MAX_TOPIC_LENGTH);
            if (topicLen == 0)
            {
                continue; // skip empty topic
            }

            uint16_t variableHeaderLen = 2 + topicLen + 2; // topic length(2) + topic + messageId(2)
            uint16_t payloadLen = outgoingMessages[i].payloadLength;
            uint16_t remainingLength = variableHeaderLen + payloadLen;

            // Build MQTT packet in buffer
            uint16_t pos = 0;

            // Write topic length MSB/LSB
            buffer[pos++] = (topicLen >> 8) & 0xFF;
            buffer[pos++] = topicLen & 0xFF;

            // Write topic string
            memcpy(buffer + pos, outgoingMessages[i].topic, topicLen);
            pos += topicLen;

            // Write message ID MSB/LSB
            buffer[pos++] = (outgoingMessages[i].msgId >> 8) & 0xFF;
            buffer[pos++] = outgoingMessages[i].msgId & 0xFF;

            // Write payload
            memcpy(buffer + pos, outgoingMessages[i].payload, payloadLen);
            pos += payloadLen;

            // Build fixed header
            size_t headerSize = buildHeader(header, buffer, pos);

            // Send the packet starting from buffer + (MQTT_MAX_HEADER_SIZE - headerSize)
            _client->write(buffer + (MQTT_MAX_HEADER_SIZE - headerSize), headerSize + pos);

            // update timestamp to avoid immediate re-retry
            outgoingMessages[i].timestamp = millis();

            lastOutActivity = millis();
        }

        // If waiting for PUBCOMP, resend PUBREL
        else if (outgoingMessages[i].state == Q2_WAIT_PUBCOMP)
        {
            // Resend PUBREL (QoS1)
            uint16_t id = outgoingMessages[i].msgId;
            this->buffer[0] = MQTTPUBREL | MQTTQOS1; // PUBREL packet with QoS 1 flag
            this->buffer[1] = 2;
            this->buffer[2] = (id >> 8);
            this->buffer[3] = (id & 0xFF);
            _client->write(this->buffer, 4);

            outgoingMessages[i].timestamp = millis();

            lastOutActivity = millis();
        }
    }
}

boolean PubSubClient::connect(const char *id)
{
    return connect(id, NULL, NULL, 0, 0, 0, 0, 1);
}

boolean PubSubClient::connect(const char *id, boolean cleanSession)
{
    return connect(id, NULL, NULL, NULL, 0, 0, NULL, cleanSession);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass)
{
    return connect(id, user, pass, 0, 0, 0, 0, 1);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass, boolean cleanSession)
{
    return connect(id, user, pass, 0, 0, 0, 0, cleanSession);
}

boolean PubSubClient::connect(const char *id, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage)
{
    return connect(id, NULL, NULL, willTopic, willQos, willRetain, willMessage, 1);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage)
{
    return connect(id, user, pass, willTopic, willQos, willRetain, willMessage, 1);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage, boolean cleanSession)
{
    if (!connected())
    {
        int result = 0;

        if (_client->connected())
        {
            result = 1;
        }
        else
        {
            if (domain != NULL)
            {
                result = _client->connect(this->domain, this->port);
            }
            else
            {
                result = _client->connect(this->ip, this->port);
            }
        }

        if (result == 1)
        {
            if (cleanSession)
            {
                nextMsgId = 1; // Reset only for clean sessions
            }
            uint16_t length = MQTT_MAX_HEADER_SIZE;
            unsigned int j;

#if MQTT_VERSION == MQTT_VERSION_3_1
            uint8_t d[9] = {0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 9
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
            const uint8_t d[7] = {0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 7
#endif
            for (j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++)
            {
                this->buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic)
            {
                v = 0x04 | (willQos << 3) | (willRetain << 5);
            }
            else
            {
                v = 0x00;
            }
            if (cleanSession)
            {
                v = v | 0x02;
            }

            if (user != NULL)
            {
                v = v | 0x80;

                if (pass != NULL)
                {
                    v = v | (0x80 >> 1);
                }
            }
            this->buffer[length++] = v;

            this->buffer[length++] = ((this->keepAlive) >> 8);
            this->buffer[length++] = ((this->keepAlive) & 0xFF);

            CHECK_STRING_LENGTH(length, id)
            length = writeString(id, this->buffer, length);
            if (willTopic)
            {
                CHECK_STRING_LENGTH(length, willTopic)
                length = writeString(willTopic, this->buffer, length);
                CHECK_STRING_LENGTH(length, willMessage)
                length = writeString(willMessage, this->buffer, length);
            }

            if (user != NULL)
            {
                CHECK_STRING_LENGTH(length, user)
                length = writeString(user, this->buffer, length);
                if (pass != NULL)
                {
                    CHECK_STRING_LENGTH(length, pass)
                    length = writeString(pass, this->buffer, length);
                }
            }

            write(MQTTCONNECT, this->buffer, length - MQTT_MAX_HEADER_SIZE);

            lastInActivity = lastOutActivity = millis();

            while (!_client->available())
            {
                unsigned long t = millis();
                if (t - lastInActivity >= ((int32_t)this->socketTimeout * 1000UL))
                {
                    _state = MQTT_CONNECTION_TIMEOUT;
                    _client->stop();
                    return false;
                }
            }
            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if (len == 4)
            {
                if (buffer[3] == 0)
                {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    _state = MQTT_CONNECTED;
                    // Resend inflight QoS 2 messages
                    if (!cleanSession)
                    {
                        resendOutgoingMessages(); // <-- only when persistent session
                    }
                    return true;
                }
                else
                {
                    _state = buffer[3];
                }
            }
            _client->stop();
        }
        else
        {
            _state = MQTT_CONNECT_FAILED;
        }
        return false;
    }
    return true;
}

// reads a byte into result
boolean PubSubClient::readByte(uint8_t *result)
{
    uint32_t previousMillis = millis();
    while (!_client->available())
    {
        yield();
        uint32_t currentMillis = millis();
        if (currentMillis - previousMillis >= ((int32_t)this->socketTimeout * 1000))
        {
            return false;
        }
    }
    *result = _client->read();
    return true;
}

// reads a byte into result[*index] and increments index
boolean PubSubClient::readByte(uint8_t *result, uint16_t *index)
{
    uint16_t current_index = *index;
    uint8_t *write_address = &(result[current_index]);
    if (readByte(write_address))
    {
        *index = current_index + 1;
        return true;
    }
    return false;
}

uint32_t PubSubClient::readPacket(uint8_t *lengthLength)
{
    uint16_t len = 0;
    if (!readByte(this->buffer, &len))
    {
        return 0;
    }
    bool isPublish = (this->buffer[0] & 0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint32_t length = 0;
    uint8_t digit = 0;
    uint16_t skip = 0;
    uint32_t start = 0;

    do
    {
        if (len == 5)
        {
            // Invalid remaining length encoding - kill the connection
            _state = MQTT_DISCONNECTED;
            _client->stop();
            return 0;
        }
        if (!readByte(&digit))
        {
            return 0;
        }
        this->buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier <<= 7; // multiplier *= 128
    } while ((digit & 128) != 0);
    *lengthLength = len - 1;

    if (isPublish)
    {
        // Read in topic length to calculate bytes to skip over for Stream writing
        if (!readByte(this->buffer, &len))
        {
            return 0;
        }
        if (!readByte(this->buffer, &len))
        {
            return 0;
        }
        skip = (this->buffer[*lengthLength + 1] << 8) + this->buffer[*lengthLength + 2];
        start = 2;
        if (this->buffer[0] & MQTTQOS1)
        {
            // skip message id
            skip += 2;
        }
    }
    uint32_t idx = len;

    for (uint32_t i = start; i < length; i++)
    {
        if (!readByte(&digit))
            return 0;
        if (this->stream)
        {
            if (isPublish && idx - *lengthLength - 2 > skip)
            {
                this->stream->write(digit);
            }
        }

        if (len < this->bufferSize)
        {
            this->buffer[len] = digit;
            len++;
        }
        idx++;
    }

    if (!this->stream && idx > this->bufferSize)
    {
        len = 0; // This will cause the packet to be ignored.
    }
    return len;
}

boolean PubSubClient::loop()
{
    if (!connected())
    {
        return false;
    }

    unsigned long t = millis();

    // Keepalive: send PINGREQ if needed
    if ((t - lastInActivity > this->keepAlive * 1000UL) || (t - lastOutActivity > this->keepAlive * 1000UL))
    {
        if (pingOutstanding)
        {
            _state = MQTT_CONNECTION_TIMEOUT;
            _client->stop();
            return false;
        }
        else
        {
            this->buffer[0] = MQTTPINGREQ;
            this->buffer[1] = 0;
            _client->write(this->buffer, 2);
            lastOutActivity = t;
            lastInActivity = t;
            pingOutstanding = true;
        }
    }

    // Retry mechanism for inflight QoS 2 messages
    unsigned long now = millis();
    const unsigned long retryInterval = 5000UL;

    for (int i = 0; i < MAX_OUTGOING_QOS2_MESSAGES; i++)
    {
        if (outgoingMessages[i].msgId == 0)
            continue;

        if ((outgoingMessages[i].state == Q2_WAIT_PUBREC || outgoingMessages[i].state == Q2_WAIT_PUBCOMP) &&
            (now - outgoingMessages[i].timestamp >= retryInterval))
        {
            if (outgoingMessages[i].state == Q2_WAIT_PUBREC)
            {
                // Resend PUBLISH with DUP=1. Make sure DUP is set before constructing header.
                uint16_t id = outgoingMessages[i].msgId;
                uint16_t length = MQTT_MAX_HEADER_SIZE;
                length = writeString(outgoingMessages[i].topic, this->buffer, length);
                if (length + 2 + outgoingMessages[i].payloadLength > this->bufferSize)
                {
                    // Message too large for buffer, skip/reschedule
                    outgoingMessages[i].timestamp = now;
                    continue;
                }
                this->buffer[length++] = (id >> 8);
                this->buffer[length++] = (id & 0xFF);
                memcpy(this->buffer + length, outgoingMessages[i].payload, outgoingMessages[i].payloadLength);
                length += outgoingMessages[i].payloadLength;
                uint8_t header = MQTTPUBLISH | (2 << 1);

                // set DUP flag (ensure it is set for this retransmission)
                outgoingMessages[i].dup = true;
                header |= 0x08;

                write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);

                // update timestamp to avoid immediate re-retry
                outgoingMessages[i].timestamp = now;
            }
            else if (outgoingMessages[i].state == Q2_WAIT_PUBCOMP)
            {
                // Resend PUBREL
                uint16_t id = outgoingMessages[i].msgId;
                this->buffer[0] = MQTTPUBREL | MQTTQOS1;
                this->buffer[1] = 2;
                this->buffer[2] = (id >> 8);
                this->buffer[3] = (id & 0xFF);
                _client->write(this->buffer, 4);

                // update timestamp to avoid immediate re-retry
                outgoingMessages[i].timestamp = now;
            }
        }
    }

    if (!_client->available())
    {
        return true;
    }

    uint8_t llen;
    uint16_t len = readPacket(&llen);

    if (len == 0)
    {
        // readPacket closed connection or error
        return false;
    }

    lastInActivity = t;

    uint8_t type = this->buffer[0] & 0xF0;

    switch (type)
    {
    case MQTTPUBLISH:
    {
        if (!callback)
            break;

        // Parse topic length
        uint16_t topicLen = (this->buffer[llen + 1] << 8) + this->buffer[llen + 2];
        // Copy topic to new buffer and null-terminate
        char topic[MAX_TOPIC_LENGTH];
        if (topicLen >= sizeof(topic))
        {
            topicLen = sizeof(topic) - 1;
        }
        memcpy(topic, this->buffer + llen + 3, topicLen);
        topic[topicLen] = '\0'; // Null terminator

        uint8_t qos = (this->buffer[0] & 0x06) >> 1;
        uint16_t msgId = 0;
        uint8_t *payload = nullptr;
        uint16_t payloadLen = 0;

        if (qos > 0)
        {
            msgId = (this->buffer[llen + 3 + topicLen] << 8) + this->buffer[llen + 3 + topicLen + 1];
            payload = this->buffer + llen + 3 + topicLen + 2;
            payloadLen = len - (llen + 3 + topicLen + 2);
        }
        else
        {
            payload = this->buffer + llen + 3 + topicLen;
            payloadLen = len - (llen + 3 + topicLen);
        }

        // Call the user callback
        if (qos == 0 || qos == 1)
        {
            if (callback)
            {
                callback(topic, payload, payloadLen);
            }
            else
            {
                break;
            }
        }

        if (qos == 1)
        {
            // Send PUBACK
            this->buffer[0] = MQTTPUBACK;
            this->buffer[1] = 2;
            this->buffer[2] = (msgId >> 8);
            this->buffer[3] = (msgId & 0xFF);
            _client->write(this->buffer, 4);
            lastOutActivity = t;
        }

        else if (qos == 2)
        {
            // Send PUBREC
            this->buffer[0] = MQTTPUBREC;
            this->buffer[1] = 2;
            this->buffer[2] = (msgId >> 8);
            this->buffer[3] = (msgId & 0xFF);
            _client->write(this->buffer, 4);
            lastOutActivity = t;

            // Check if already stored (duplicate PUBLISH)
            bool alreadyStored = false;
            for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; i++)
            {
                if (incomingMessages[i].msgId == msgId &&
                    incomingMessages[i].state == Q2_RECEIVED_PUBLISH)
                {
                    alreadyStored = true;
                    break;
                }
            }

            if (!alreadyStored)
            {
                // Store in first free slot
                for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; i++)
                {
                    if (incomingMessages[i].state == Q2_IDLE)
                    {
                        incomingMessages[i].msgId = msgId;
                        incomingMessages[i].state = Q2_RECEIVED_PUBLISH;
                        // Make sure to null-terminate topic when copying
                        strncpy(incomingMessages[i].topic, topic, MAX_TOPIC_LENGTH - 1);
                        incomingMessages[i].topic[MAX_TOPIC_LENGTH - 1] = '\0';
                        memcpy(incomingMessages[i].payload, payload, payloadLen);
                        incomingMessages[i].payloadLength = payloadLen;
                        break;
                    }
                }
            }
        }
        break;
    }

    case MQTTPINGREQ:
    {
        // Respond with PINGRESP
        this->buffer[0] = MQTTPINGRESP;
        this->buffer[1] = 0;
        _client->write(this->buffer, 2);
        break;
    }

    case MQTTPINGRESP:
    {
        // Got PINGRESP, clear pingOutstanding
        pingOutstanding = false;
        break;
    }

    case MQTTPUBREC:
    {
        uint16_t msgId = (this->buffer[2] << 8) + this->buffer[3];

        // Update inflight message state to Q2_WAIT_PUBCOMP for this msgId
        for (int i = 0; i < MAX_OUTGOING_QOS2_MESSAGES; i++)
        {
            if (outgoingMessages[i].msgId == msgId && outgoingMessages[i].state == Q2_WAIT_PUBREC)
            {
                outgoingMessages[i].state = Q2_WAIT_PUBCOMP;
                break;
            }
        }

        // Send PUBREL
        this->buffer[0] = MQTTPUBREL | MQTTQOS1; // PUBREL packet with QoS 1 flag
        this->buffer[1] = 2;
        this->buffer[2] = (msgId >> 8);
        this->buffer[3] = (msgId & 0xFF);
        _client->write(this->buffer, 4);
        lastOutActivity = t;
        break;
    }

    case MQTTPUBREL:
    {
        uint16_t msgId = (this->buffer[2] << 8) + this->buffer[3];

        for (int i = 0; i < MAX_INCOMING_QOS2_MESSAGES; i++)
        {
            if (incomingMessages[i].msgId == msgId &&
                incomingMessages[i].state == Q2_RECEIVED_PUBLISH)
            {
                // Deliver to callback only once
                if (callback)
                {
                    callback(incomingMessages[i].topic,
                             incomingMessages[i].payload,
                             incomingMessages[i].payloadLength);
                }

                // Clean up
                incomingMessages[i].state = Q2_IDLE;
                incomingMessages[i].msgId = 0;
                incomingMessages[i].payloadLength = 0;
                incomingMessages[i].topic[0] = '\0';
                break;
            }
        }

        // Send PUBCOMP regardless
        this->buffer[0] = MQTTPUBCOMP;
        this->buffer[1] = 2;
        this->buffer[2] = (msgId >> 8);
        this->buffer[3] = (msgId & 0xFF);
        _client->write(this->buffer, 4);
        lastOutActivity = t;
        break;
    }

    case MQTTPUBCOMP:
    {
        uint16_t msgId = (this->buffer[2] << 8) + this->buffer[3];

        // Mark inflight message as done and clear msgId
        for (int i = 0; i < MAX_OUTGOING_QOS2_MESSAGES; i++)
        {
            if (outgoingMessages[i].msgId == msgId && outgoingMessages[i].state == Q2_WAIT_PUBCOMP)
            {
                outgoingMessages[i].state = Q2_DONE;
                outgoingMessages[i].msgId = 0;
                outgoingMessages[i].payloadLength = 0;
                outgoingMessages[i].dup = false;
                memset(outgoingMessages[i].topic, 0, sizeof(outgoingMessages[i].topic));
                memset(outgoingMessages[i].payload, 0, sizeof(outgoingMessages[i].payload));
                break;
            }
        }
        // QoS 2 handshake fully completed for this message
        break;
    }

    default:
        // Unknown packet type — ignore or handle if needed
        break;
    }
    return true;
}

boolean PubSubClient::publish(const char *topic, const char *payload)
{
    return publish(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, false);
}

boolean PubSubClient::publish(const char *topic, const char *payload, boolean retained)
{
    return publish(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, retained);
}

boolean PubSubClient::publish(const char *topic, const uint8_t *payload, unsigned int plength)
{
    return publish(topic, payload, plength, false);
}

boolean PubSubClient::publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
    if (connected())
    {
        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2 + strnlen(topic, this->bufferSize) + plength)
        {
            // Too long
            return false;
        }
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, this->buffer, length);

        // Add payload
        uint16_t i;
        for (i = 0; i < plength; i++)
        {
            this->buffer[length++] = payload[i];
        }

        // Write the header
        uint8_t header = MQTTPUBLISH;
        if (retained)
        {
            header |= 1;
        }
        return write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

// Overload publish for QoS 2 support
boolean PubSubClient::publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained, uint8_t qos)
{
    if (qos == 0)
    {
        // Use the original implementation for QoS 0
        return publish(topic, payload, retained);
    }
    if (!connected() || qos > 2 || plength > MAX_PAYLOAD_LENGTH || strlen(topic) >= MAX_TOPIC_LENGTH)
        return false;

    uint16_t length = MQTT_MAX_HEADER_SIZE;
    length = writeString(topic, this->buffer, length);

    uint16_t id = 0;
    if (qos > 0)
    {
        nextMsgId = (nextMsgId == 0) ? 1 : nextMsgId + 1;
        id = nextMsgId;
        this->buffer[length++] = (id >> 8);
        this->buffer[length++] = (id & 0xFF);
    }

    for (unsigned int i = 0; i < plength; i++)
        this->buffer[length++] = payload[i];

    uint8_t header = MQTTPUBLISH | (qos << 1);
    if (retained)
        header |= 0x01;

    if (!write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE))
        return false;

    if (qos == 2)
    {
        for (int i = 0; i < MAX_OUTGOING_QOS2_MESSAGES; i++)
        {
            if (outgoingMessages[i].state == Q2_DONE || outgoingMessages[i].msgId == 0)
            {
                outgoingMessages[i].msgId = id;
                outgoingMessages[i].state = Q2_WAIT_PUBREC;
                outgoingMessages[i].timestamp = millis();
                // copy topic and ensure null termination
                strncpy(outgoingMessages[i].topic, topic, MAX_TOPIC_LENGTH - 1);
                outgoingMessages[i].topic[MAX_TOPIC_LENGTH - 1] = '\0';
                memcpy(outgoingMessages[i].payload, payload, plength);
                outgoingMessages[i].payloadLength = plength;
                outgoingMessages[i].dup = false;
                break;
            }
        }
    }

    return true;
}

// Overload publish for QoS 2 support
boolean PubSubClient::publish(const char *topic, const char *payload, uint8_t qos)
{
    if (qos == 0)
    {
        // Use the original implementation for QoS 0
        return publish(topic, payload);
    }
    return publish(topic, (const uint8_t *)payload, strlen(payload), false, qos);
}

// Overload publish for QoS 2 support
boolean PubSubClient::publish(const char *topic, const char *payload, boolean retained, uint8_t qos)
{
    if (qos == 0)
    {
        // Use the original implementation for QoS 0
        return publish(topic, payload, retained);
    }
    return publish(topic, (const uint8_t *)payload, strlen(payload), retained, qos);
}

// Overload publish for QoS 2 support
boolean PubSubClient::publish(const char *topic, const char *payload, QOS qos)
{
    if (qos == 0)
    {
        // Use the original implementation for QoS 0
        return publish(topic, payload);
    }
    return publish(topic, payload, static_cast<uint8_t>(qos));
}

boolean PubSubClient::publish_P(const char *topic, const char *payload, boolean retained)
{
    return publish_P(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, retained);
}

boolean PubSubClient::publish_P(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
    uint8_t llen = 0;
    uint8_t digit;
    unsigned int rc = 0;
    uint16_t tlen;
    unsigned int pos = 0;
    unsigned int i;
    uint8_t header;
    unsigned int len;
    int expectedLength;

    if (!connected())
    {
        return false;
    }

    tlen = strnlen(topic, this->bufferSize);

    header = MQTTPUBLISH;
    if (retained)
    {
        header |= 1;
    }
    this->buffer[pos++] = header;
    len = plength + 2 + tlen;
    do
    {
        digit = len & 127; // digit = len %128
        len >>= 7;         // len = len / 128
        if (len > 0)
        {
            digit |= 0x80;
        }
        this->buffer[pos++] = digit;
        llen++;
    } while (len > 0);

    pos = writeString(topic, this->buffer, pos);

    rc += _client->write(this->buffer, pos);

    for (i = 0; i < plength; i++)
    {
        rc += _client->write((char)pgm_read_byte_near(payload + i));
    }

    lastOutActivity = millis();

    expectedLength = 1 + llen + 2 + tlen + plength;

    return (rc == expectedLength);
}

boolean PubSubClient::beginPublish(const char *topic, unsigned int plength, boolean retained)
{
    if (connected())
    {
        // Send the header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, this->buffer, length);
        uint8_t header = MQTTPUBLISH;
        if (retained)
        {
            header |= 1;
        }
        size_t hlen = buildHeader(header, this->buffer, plength + length - MQTT_MAX_HEADER_SIZE);
        uint16_t rc = _client->write(this->buffer + (MQTT_MAX_HEADER_SIZE - hlen), length - (MQTT_MAX_HEADER_SIZE - hlen));
        lastOutActivity = millis();
        return (rc == (length - (MQTT_MAX_HEADER_SIZE - hlen)));
    }
    return false;
}

int PubSubClient::endPublish()
{
    return 1;
}

size_t PubSubClient::write(uint8_t data)
{
    lastOutActivity = millis();
    return _client->write(data);
}

size_t PubSubClient::write(const uint8_t *buffer, size_t size)
{
    lastOutActivity = millis();
    return _client->write(buffer, size);
}

size_t PubSubClient::buildHeader(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    do
    {

        digit = len & 127; // digit = len %128
        len >>= 7;         // len = len / 128
        if (len > 0)
        {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while (len > 0);

    buf[4 - llen] = header;
    for (int i = 0; i < llen; i++)
    {
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

boolean PubSubClient::write(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE
    uint8_t *writeBuf = buf + (MQTT_MAX_HEADER_SIZE - hlen);
    uint16_t bytesRemaining = length + hlen; // Match the length type
    uint8_t bytesToWrite;
    boolean result = true;
    while ((bytesRemaining > 0) && result)
    {
        bytesToWrite = (bytesRemaining > MQTT_MAX_TRANSFER_SIZE) ? MQTT_MAX_TRANSFER_SIZE : bytesRemaining;
        rc = _client->write(writeBuf, bytesToWrite);
        result = (rc == bytesToWrite);
        bytesRemaining -= rc;
        writeBuf += rc;
    }
    return result;
#else
    rc = _client->write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
    lastOutActivity = millis();
    return (rc == hlen + length);
#endif
}

boolean PubSubClient::subscribe(const char *topic)
{
    return subscribe(topic, 0);
}

boolean PubSubClient::subscribe(const char *topic, uint8_t qos)
{
    size_t topicLength = strnlen(topic, this->bufferSize);
    if (topic == 0)
    {
        return false;
    }
    if (qos > 2)
    {
        return false;
    }
    if (this->bufferSize < 9 + topicLength)
    {
        // Too long
        return false;
    }
    if (connected())
    {
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0)
        {
            nextMsgId = 1;
        }
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xFF);
        length = writeString((char *)topic, this->buffer, length);
        this->buffer[length++] = qos;
        return write(MQTTSUBSCRIBE | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

boolean PubSubClient::unsubscribe(const char *topic)
{
    size_t topicLength = strnlen(topic, this->bufferSize);
    if (topic == 0)
    {
        return false;
    }
    if (this->bufferSize < 9 + topicLength)
    {
        // Too long
        return false;
    }
    if (connected())
    {
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0)
        {
            nextMsgId = 1;
        }
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xFF);
        length = writeString(topic, this->buffer, length);
        return write(MQTTUNSUBSCRIBE | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

void PubSubClient::disconnect()
{
    this->buffer[0] = MQTTDISCONNECT;
    this->buffer[1] = 0;
    _client->write(this->buffer, 2);
    _state = MQTT_DISCONNECTED;
    _client->flush();
    _client->stop();
    lastInActivity = lastOutActivity = millis();
}

uint16_t PubSubClient::writeString(const char *string, uint8_t *buf, uint16_t pos)
{
    const char *idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp)
    {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos - i - 2] = (i >> 8);
    buf[pos - i - 1] = (i & 0xFF);
    return pos;
}

boolean PubSubClient::connected()
{
    boolean rc;
    if (_client == NULL)
    {
        rc = false;
    }
    else
    {
        rc = (int)_client->connected();
        if (!rc)
        {
            if (this->_state == MQTT_CONNECTED)
            {
                this->_state = MQTT_CONNECTION_LOST;
                _client->flush();
                _client->stop();
            }
        }
        else
        {
            return this->_state == MQTT_CONNECTED;
        }
    }
    return rc;
}

PubSubClient &PubSubClient::setServer(const uint8_t *ip, uint16_t port)
{
    IPAddress addr(ip[0], ip[1], ip[2], ip[3]);
    return setServer(addr, port);
}

PubSubClient &PubSubClient::setServer(IPAddress ip, uint16_t port)
{
    this->ip = ip;
    this->port = port;
    this->domain = NULL;
    return *this;
}

PubSubClient &PubSubClient::setServer(const char *domain, uint16_t port)
{
    this->domain = domain;
    this->port = port;
    return *this;
}

PubSubClient &PubSubClient::setCallback(MQTT_CALLBACK_SIGNATURE)
{
    this->callback = callback;
    return *this;
}

PubSubClient &PubSubClient::setClient(Client &client)
{
    this->_client = &client;
    return *this;
}

PubSubClient &PubSubClient::setStream(Stream &stream)
{
    this->stream = &stream;
    return *this;
}

int PubSubClient::state()
{
    return this->_state;
}

boolean PubSubClient::setBufferSize(uint16_t size)
{
    if (size == 0)
    {
        // Cannot set it back to 0
        return false;
    }
    if (this->bufferSize == 0)
    {
        this->buffer = (uint8_t *)malloc(size);
    }
    else
    {
        uint8_t *newBuffer = (uint8_t *)realloc(this->buffer, size);
        if (newBuffer != NULL)
        {
            this->buffer = newBuffer;
        }
        else
        {
            return false;
        }
    }
    this->bufferSize = size;
    return (this->buffer != NULL);
}

uint16_t PubSubClient::getBufferSize()
{
    return this->bufferSize;
}
PubSubClient &PubSubClient::setKeepAlive(uint16_t keepAlive)
{
    this->keepAlive = keepAlive;
    return *this;
}
PubSubClient &PubSubClient::setSocketTimeout(uint16_t timeout)
{
    this->socketTimeout = timeout;
    return *this;
}
