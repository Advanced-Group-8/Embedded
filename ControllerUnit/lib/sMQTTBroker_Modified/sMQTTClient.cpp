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

static uint16_t lastQos2MsgId = 0;
static bool lastQos2Processed = false;

void sMQTTClient::processMessage()
{
	if (message.type() <= sMQTTMessage::Type::Disconnect)
	{
		SMQTT_LOGD("message type:%s(0x%x)", debugMessageType[message.type() / 0x10], message.type());
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

		unsigned short len;
		const char *payload = header;
		message.getString(payload, len);

		const char *topicName = payload;
		std::string _topicName(topicName, len);
		payload += len;

		char mqttPacketId[2] = {0};
		uint16_t msgId = 0;
		if (qos)
		{
			mqttPacketId[0] = payload[0];
			mqttPacketId[1] = payload[1];
			payload += 2;
			msgId = (static_cast<uint8_t>(mqttPacketId[0]) << 8) | static_cast<uint8_t>(mqttPacketId[1]);
		}
		len = message.end() - payload;
		std::string _payload(payload, len);

		bool process = true;
		if (qos == 2)
		{
			if (msgId == lastQos2MsgId && lastQos2Processed)
			{
				process = false;
			}
			else
			{
				lastQos2MsgId = msgId;
				lastQos2Processed = true;
			}
		}

		if (process)
		{
			sMQTTTopic topic(_topicName, _payload, qos);
			if (message.isRetained())
				_parent->updateRetainedTopic(&topic);
			_parent->publish(this, &topic, &message);
		}

		if (qos == 2)
		{
			char pubrec[4] = {0x50, 0x02, static_cast<char>(msgId >> 8), static_cast<char>(msgId & 0xFF)};
			write(pubrec, sizeof(pubrec));
		}
		else if (qos == 1)
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
			char pubcomp[4] = {0x70, 0x02, static_cast<char>(msgId >> 8), static_cast<char>(msgId & 0xFF)};
			write(pubcomp, sizeof(pubcomp));
			lastQos2Processed = false;
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
		// (subscription handling) ...
	}
	break;

	case sMQTTMessage::Type::UnSubscribe:
	{
		// (unsubscription handling) ...
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
