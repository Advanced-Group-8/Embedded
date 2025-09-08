#include "sMQTTBroker.h"

bool sMQTTBroker::init(unsigned short port, bool checkWifiConnection)
{
	isCheckWifiConnection = checkWifiConnection;
	_server = new TCPServer(port);
	if (_server == 0)
		return false;
	_server->begin();
	return true;
};

void sMQTTBroker::update()
{
#if defined(ESP8266) || defined(ESP32) || defined(WIO_TERMINAL)
	if (isCheckWifiConnection)
	{
		if (WiFi.isConnected() == false)
		{
			sMQTTLostConnectionEvent event;
			onEvent(&event);
			return;
		}
	}
	TCPClient client = _server->available();
	if (client)
	{
		SMQTT_LOGD("New Client");
		sMQTTClient *sClient = new sMQTTClient(this, client);
		clients.push_back(sClient);
	}
#endif
	sMQTTClientList::iterator clit;
	for (clit = clients.begin(); clit != clients.end(); clit++)
	{
		sMQTTClient *c = *clit;
		if (c->isConnected())
			c->update();
		else
		{

			sMQTTRemoveClientEvent event(c);
			onEvent(&event);

			for (sMQTTTopicList::iterator sub = subscribes.begin(); sub != subscribes.end(); sub++)
			{
				if ((*sub)->unsubscribe(c) == true)
				{
					delete *sub;
					sub = subscribes.erase(sub);
					if (sub == subscribes.end())
						break;
					sub--;
				}
			}

			delete c;
			clit = clients.erase(clit);
			SMQTT_LOGD("Clients %d", clients.size());
			if (clit == clients.end())
				break;
			clit--;
		}
	}
};

bool sMQTTBroker::subscribe(sMQTTClient *client, const char *topic)
{
	if (isTopicValidName(topic) == false)
		return false;

	sMQTTSubUnSubClientEvent event(Subscribe_sMQTTEventType, client, topic);
	onEvent(&event);

	sMQTTTopicList::iterator sub;
	for (sub = subscribes.begin(); sub != subscribes.end(); sub++)
	{
		if (strcmp((*sub)->Name(), topic) == 0)
		{
			(*sub)->subscribe(client);
			findRetainTopic(*sub, client);
			return true;
		}
	}
	sMQTTTopic *newTopic = new sMQTTTopic(topic, 0);
	newTopic->subscribe(client);
	subscribes.push_back(newTopic);

	findRetainTopic(newTopic, client);
	return true;
};

void sMQTTBroker::unsubscribe(sMQTTClient *client, const char *topic)
{
	sMQTTSubUnSubClientEvent event(UnSubscribe_sMQTTEventType, client, topic);
	onEvent(&event);

	sMQTTTopicList::iterator it;
	for (it = subscribes.begin(); it != subscribes.end(); it++)
	{
		if (strcmp((*it)->Name(), topic) == 0)
		{
			if ((*it)->unsubscribe(client) == true)
			{
				delete *it;
				subscribes.erase(it);
			}
			break;
		}
	}
};

void sMQTTBroker::publish(sMQTTClient *client, sMQTTTopic *topic, sMQTTMessage *msg)
{
	SMQTT_LOGD("Broker::publish QoS=%d topic=%s",
			   msg ? msg->QoS() : -1,
			   topic ? topic->Name() : "(null)");

	sMQTTPublicClientEvent event(client, std::string(topic->Name()));
	if (topic->Payload())
		event.setPayload(std::string(topic->Payload()));
	onEvent(&event);

	if (msg && client && msg->QoS() == 1)
	{
		uint16_t msgId = msg->getMessageId();
		SMQTT_LOGD("PUBACK: Received PUBLISH with msgId=%u\n", msgId);
		uint8_t puback[4] = {0x40, 0x02,
							 static_cast<uint8_t>(msgId >> 8),
							 static_cast<uint8_t>(msgId & 0xFF)};
		SMQTT_LOGD("PUBACK: Sending PUBACK with msgId=%u (bytes: 0x%02X 0x%02X)\n", msgId, puback[2], puback[3]);
		client->write(reinterpret_cast<const char *>(puback), sizeof(puback));
		client->flushSocket();
	}

	// Forward to all matching subscribers
	sMQTTTopicList::iterator sub;
	for (sub = subscribes.begin(); sub != subscribes.end(); sub++)
	{
		if ((*sub)->match(topic->Name()))
		{
			sMQTTClientList subList = (*sub)->getSubscribeList();
			SMQTT_LOGD("Forwarding topic %s to %d client(s)",
					   topic->Name(), subList.size());

			bool first = true;
			for (auto cl : subList)
			{
				if (first)
				{
					msg->sendTo(cl); // recalc remaining length
					first = false;
				}
				else
				{
					msg->sendTo(cl, false); // reuse
				}
			}
		}
	}
};

bool sMQTTBroker::isTopicValidName(const char *filter)
{
	int length = strlen(filter);
	const char *hashpos = strchr(filter, '#');

	if (hashpos != NULL)
	{
		const char *second = strchr(hashpos + 1, '#');
		if ((hashpos != filter && hashpos != filter + (length - 1)) || second != NULL)
			return false;
	}
	for (const char *c = "#+"; *c != '\0'; ++c)
	{
		const char *pos = strchr(filter, *c);
		while (pos != NULL)
		{
			if (pos > filter)
			{
				if (*(pos - 1) != '/')
					return false;
			}
			if (*(pos + 1) != '\0')
			{
				if (*(pos + 1) != '/')
					return false;
			}
			pos = strchr(pos + 1, *c);
		}
	}
	return true;
};

void sMQTTBroker::updateRetainedTopic(sMQTTTopic *topic)
{
	SMQTT_LOGD("updateRetainedTopic %s", topic->Name());
	sMQTTTopicList::iterator it;
	for (it = retains.begin(); it != retains.end(); it++)
	{
		if (strcmp((*it)->Name(), topic->Name()) == 0)
			break;
	}
	if (it != retains.end())
	{
		SMQTT_LOGD("updateRetainedTopic update %s", topic->Name());
		if (topic->Payload())
			(*it)->update(topic);
		else
		{
			SMQTT_LOGD("updateRetainedTopic delete %s", topic->Name());
			delete *it;
			retains.erase(it);
		}
	}
	else
	{
		if (topic->Payload())
		{
			sMQTTTopic *newTopic = new sMQTTTopic(topic);
			retains.push_back(newTopic);
		}
	}
};

void sMQTTBroker::findRetainTopic(sMQTTTopic *topic, sMQTTClient *client)
{
	sMQTTTopicList::iterator it;
	unsigned long time = 0;
	for (it = retains.begin(); it != retains.end(); it++)
	{
		if (topic->match((*it)->Name()))
		{
			SMQTT_LOGD("findRetainTopic %s qos:%d",
					   (*it)->Name(), (*it)->QoS());
			SMQTT_LOGD("findRetainTopic %s", (*it)->Payload());
			sMQTTMessage msg(sMQTTMessage::Type::Publish, (*it)->QoS() << 1);
			msg.add((*it)->Name(), strlen((*it)->Name()));
			if ((*it)->QoS())
			{
				msg.add(time >> 8);
				msg.add(time);
				time++;
			}
			msg.add((*it)->Payload(), strlen((*it)->Payload()), false);
			msg.sendTo(client);
		}
	}
};

bool sMQTTBroker::isClientConnected(sMQTTClient *client)
{
	sMQTTClientList::iterator clit;
	for (clit = clients.begin(); clit != clients.end(); clit++)
	{
		sMQTTClient *c = *clit;
		if (c == client)
			return false;
		if (c->getClientId() == client->getClientId())
		{
			SMQTT_LOGD("found:%s client size:%d",
					   client->getClientId().c_str(), clients.size());
			return true;
		}
	}
	return false;
};

void sMQTTBroker::publish(const std::string &topic,
						  const std::string &payload,
						  unsigned char qos,
						  bool retain)
{
	// static counter for msg ids (wraparound)
	static uint16_t msg_id_counter = 1;

	// allocate one msg_id per publish call (will be used for all subscribers of this publish)
	uint16_t msg_id = msg_id_counter++;
	if (msg_id == 0)
		msg_id = msg_id_counter++;

	for (auto sub = subscribes.begin(); sub != subscribes.end(); ++sub)
	{
		if ((*sub)->match(topic))
		{
			sMQTTClientList subList = (*sub)->getSubscribeList();
			SMQTT_LOGD("topic %s Clients %d",
					   topic.c_str(), subList.size());

			for (auto cl : subList)
			{
				// Build a per-client message instance (so each client can track inflight separately)
				sMQTTMessage msg(sMQTTMessage::Type::Publish, qos << 1);
				msg.add(topic.c_str(), topic.size());

				// add message id if qos > 0
				if (qos == 1)
				{
					msg.add(static_cast<char>(msg_id >> 8));
					msg.add(static_cast<char>(msg_id & 0xFF));
				}

				msg.add(payload.c_str(), payload.size(), false);

				if (qos == 1)
				{
					// store in client's inflight map for retransmit until PUBACK
					cl->inflight[msg_id] = msg;

#if defined(ESP8266) || defined(ESP32)
					cl->inflight[msg_id].timestamp = millis();
#else
					cl->inflight[msg_id].timestamp = 0;
#endif
				}
				msg.sendTo(cl);
			}
		}
	}

	if (retain)
	{
		sMQTTTopic Topic((std::string &)topic,
						 (std::string &)payload, qos);
		updateRetainedTopic(&Topic);
	}
};

void sMQTTBroker::restart()
{
#if defined(ESP8266)
	_server->stop();
#else
	_server->end();
#endif
	_server->begin();
};

unsigned long sMQTTBroker::getRetainedTopicCount()
{
	return retains.size();
};

std::string sMQTTBroker::getRetaiedTopicName(unsigned long index)
{
	if (index >= retains.size())
		return "";
	return retains[index]->Name();
};
