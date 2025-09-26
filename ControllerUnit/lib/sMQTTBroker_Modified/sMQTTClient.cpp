#include "sMQTTBroker.h"

sMQTTClient::sMQTTClient(sMQTTBroker *parent, TCPClient &client)
	: mqtt_connected(false), _parent(parent)
{
	_client = client;
	keepAlive = 25;
	updateLiveStatus();
};

sMQTTClient::~sMQTTClient()
{
	SMQTT_LOGD("Broker: sMQTTClient destructor called, connection closed.\n");
};

void sMQTTClient::update()
{
	while (_client.available() > 0)
	{
		message.incoming(_client.read());
		if (message.type())
		{
			processMessage();
			message.reset();
			break;
		}
	}

	// Reap stale QoS2 pending publishes (no PUBREL received)
#if defined(ESP8266) || defined(ESP32)
	unsigned long nowMs = millis();
	for (auto it = pendingQos2.begin(); it != pendingQos2.end();)
	{
		if (nowMs - it->second.storedAt > qos2TimeoutMs)
		{
			SMQTT_LOGD("QoS2 timeout(%lums): dropping msgId=%u topic='%s'\n", qos2TimeoutMs, it->first, it->second.topic.c_str());
			it = pendingQos2.erase(it);
		}
		else
		{
			++it;
		}
	}
#endif

#if defined(ESP8266) || defined(ESP32)
	unsigned long now = millis();
	for (auto it = inflight.begin(); it != inflight.end();)
	{
		if (now - it->second.timestamp > 5000)
		{
			SMQTT_LOGD("Resending QoS1 msgId=%u", it->first);
			it->second.sendTo(this, false);
			it->second.timestamp = now;
		}
		++it;
	}
#endif

	unsigned long currentMillis;
#if defined(ESP8266) || defined(ESP32)
	currentMillis = millis();
#endif
	if (keepAlive != 0 && aliveMillis < currentMillis)
	{
		SMQTT_LOGD("Broker: Keepalive timeout, closing connection. aliveMillis(%lu) < currentMillis(%lu)\n", aliveMillis, currentMillis);
		_client.stop();
	}
};

bool sMQTTClient::isConnected()
{
	return _client.connected();
};

void sMQTTClient::write(const char *buf, size_t length)
{
	_client.write(buf, length);
}

void sMQTTClient::processMessage()
{
	if (message.type() <= sMQTTMessage::Type::Disconnect)
	{
		SMQTT_LOGD("Message type:%s(0x%x)", debugMessageType[message.type() / 0x10], message.type());
	}

	const char *header = message.getVHeader();
	switch (message.type())
	{
	case sMQTTMessage::Type::Connect:
	{
		char connack[4] = {0x20, 0x02, 0x00, 0x00};
		write(connack, sizeof(connack));
		mqtt_connected = true;
	}
	break;

	case sMQTTMessage::Type::Publish:
	{
		unsigned char qos = message.QoS();

		unsigned short topicLen;
		const char *cursor = header;
		message.getString(cursor, topicLen);

		const char *frameEnd = message.end();
		if (cursor + topicLen > frameEnd || topicLen == 0 || topicLen > 256)
		{
			SMQTT_LOGD("Malformed PUBLISH: invalid topic length=%u (frame size=%u)\n", topicLen, (unsigned)(frameEnd - header));
			break;
		}
		std::string topicStr(cursor, topicLen);
		cursor += topicLen;

		uint16_t msgId = 0;
		if (qos)
		{
			if (cursor + 2 > frameEnd)
			{
				SMQTT_LOGD("Malformed PUBLISH: missing packet id bytes\n");
				break;
			}
			msgId = ((uint8_t)cursor[0] << 8) | (uint8_t)cursor[1];
			cursor += 2;
		}

		if (cursor > frameEnd)
		{
			SMQTT_LOGD("Malformed PUBLISH: header overflow after msgId\n");
			break;
		}

		std::string payloadStr(cursor, frameEnd - cursor);
		SMQTT_LOGD("PUBLISH parsed: topic='%s' payloadLen=%u qos=%u msgId=%u\n", topicStr.c_str(), (unsigned)payloadStr.size(), qos, msgId);

		if (qos == 2)
		{
			if (pendingQos2.size() >= PENDING_QOS2_MAX)
			{
				SMQTT_LOGD("QoS2 overflow: rejecting new msgId=%u (limit=%u)\n", msgId, (unsigned)PENDING_QOS2_MAX);
				_client.stop();
				break;
			}
			pendingQos2[msgId] = {topicStr, payloadStr, message.isRetained(), 2,
#if defined(ESP8266) || defined(ESP32)
								  millis()
#else
								  0
#endif
			};
			char pubrec[4] = {0x50, 0x02, static_cast<char>(msgId >> 8), static_cast<char>(msgId & 0xFF)};
			write(pubrec, sizeof(pubrec));
			break;
		}

		// QoS 0 or 1: deliver immediately
		sMQTTTopic topic(topicStr, payloadStr, qos);
		if (message.isRetained())
			_parent->updateRetainedTopic(&topic);
		_parent->publish(this, &topic, &message);

		if (qos == 1)
		{
			char puback[4] = {0x40, 0x02, static_cast<char>(msgId >> 8), static_cast<char>(msgId & 0xFF)};
			write(puback, sizeof(puback));
		}
	}
	break;

	case sMQTTMessage::Type::PubAck:
	{
		uint16_t msgId = (static_cast<uint8_t>(header[0]) << 8) |
						 static_cast<uint8_t>(header[1]);
		SMQTT_LOGD("Client: Received PUBACK for msgId=%u\n", msgId);
		auto it = inflight.find(msgId);
		if (it != inflight.end())
		{
			SMQTT_LOGD("Client: Removing inflight msgId=%u\n", msgId);
			inflight.erase(it);
		}
	}
	break;
	case sMQTTMessage::Type::PubRel:
	{
		if (header)
		{
			uint16_t msgId = (static_cast<uint8_t>(header[0]) << 8) | static_cast<uint8_t>(header[1]);
			auto it = pendingQos2.find(msgId);
			if (it != pendingQos2.end())
			{
				SMQTT_LOGD("PUBREL: delivering stored QoS2 msgId=%u topic='%s' payloadLen=%u\n", msgId, it->second.topic.c_str(), (unsigned)it->second.payload.size());
				sMQTTTopic topic(it->second.topic, it->second.payload, 2);
				if (it->second.retain)
					_parent->updateRetainedTopic(&topic);
				_parent->publish(this, &topic, &message);
				pendingQos2.erase(it);
			}
			else
			{
				SMQTT_LOGD("PUBREL: unknown msgId=%u (duplicate or expired)\n", msgId);
			}
			char pubcomp[4] = {0x70, 0x02, static_cast<char>(msgId >> 8), static_cast<char>(msgId & 0xFF)};
			write(pubcomp, sizeof(pubcomp));
		}
	}
	break;

	case sMQTTMessage::Type::PubComp:
	{
		// QoS2 finalization
	}
	break;

	case sMQTTMessage::Type::Subscribe:
	{
		// MQTT SUBSCRIBE Variable header: packet identifier (2 bytes)
		const char *ptr = header;
		if (message.end() - ptr < 2)
		{
			SMQTT_LOGD("SUBSCRIBE malformed: no packet id\n");
			break;
		}
		uint16_t subMsgId = ((uint8_t)ptr[0] << 8) | (uint8_t)ptr[1];
		ptr += 2;
		// Payload: topic filters + QoS byte each
		std::vector<uint8_t> grantedQoS;
		while (ptr < message.end())
		{
			if (message.end() - ptr < 3)
			{
				SMQTT_LOGD("SUBSCRIBE malformed: remaining <3 bytes\n");
				break;
			}
			unsigned short tLen = ((uint8_t)ptr[0] << 8) | (uint8_t)ptr[1];
			ptr += 2;
			if (ptr + tLen + 1 > message.end())
			{
				SMQTT_LOGD("SUBSCRIBE malformed: topic length overflow\n");
				break;
			}
			std::string filter(ptr, tLen);
			ptr += tLen;
			uint8_t reqQos = (uint8_t)*ptr++ & 0x03;
			// Accept all filters for now
			_parent->subscribe(this, filter.c_str());
			grantedQoS.push_back(reqQos); // echo requested QoS
			SMQTT_LOGD("SUBSCRIBE: filter='%s' reqQos=%u\n", filter.c_str(), reqQos);
		}
		// Build SUBACK
		sMQTTMessage suback(sMQTTMessage::Type::SubAck);
		suback.add(subMsgId >> 8);
		suback.add(subMsgId & 0xFF);
		for (auto q : grantedQoS)
			suback.add((char)q);
		suback.sendTo(this);
	}
	break;

	case sMQTTMessage::Type::UnSubscribe:
	{
		const char *ptr = header;
		if (message.end() - ptr < 2)
		{
			SMQTT_LOGD("UNSUBSCRIBE malformed: no packet id\n");
			break;
		}
		uint16_t unsubMsgId = ((uint8_t)ptr[0] << 8) | (uint8_t)ptr[1];
		ptr += 2;
		while (ptr < message.end())
		{
			if (message.end() - ptr < 2)
			{
				break;
			}
			unsigned short tLen = ((uint8_t)ptr[0] << 8) | (uint8_t)ptr[1];
			ptr += 2;
			if (ptr + tLen > message.end())
			{
				break;
			}
			std::string filter(ptr, tLen);
			ptr += tLen;
			_parent->unsubscribe(this, filter.c_str());
			SMQTT_LOGD("UNSUBSCRIBE: filter='%s'\n", filter.c_str());
		}
		sMQTTMessage unsuback(sMQTTMessage::Type::UnSuback);
		unsuback.add(unsubMsgId >> 8);
		unsuback.add(unsubMsgId & 0xFF);
		unsuback.sendTo(this);
	}
	break;

	case sMQTTMessage::Type::Disconnect:
	{
		SMQTT_LOGD("Broker: Client requested disconnect, closing connection.\n");
		mqtt_connected = false;
		_client.stop();
	}
	break;

	case sMQTTMessage::Type::PingReq:
	{
		SMQTT_LOGD("Broker: Received PINGREQ from client.\n");
		sMQTTMessage msg(sMQTTMessage::Type::PingResp);
		msg.sendTo(this);
	}
	break;

	default:
	{
		SMQTT_LOGD("Broker: unknown message %d, closing connection.\n", message.type());
		mqtt_connected = false;
		_client.stop();
	}
	break;
	}
	updateLiveStatus();
};

void sMQTTClient::updateLiveStatus()
{
	if (keepAlive)
#if defined(ESP8266) || defined(ESP32)
		aliveMillis = keepAlive * 1500 + millis();
#else
		aliveMillis = 0;
#endif
	else
		aliveMillis = 0;
}
