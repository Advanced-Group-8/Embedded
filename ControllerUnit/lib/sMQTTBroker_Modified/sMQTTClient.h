#ifndef SMQTTCLIENT_FILE
#define SMQTTCLIENT_FILE

#include "sMQTTMessage.h"
#include <map>
#include <unordered_map>

class sMQTTBroker;

#define sMQTTUserNameFlag 0x80
#define sMQTTPasswordFlag 0x40
#define sMQTTWillRetainFlag 0x20
#define sMQTTWillQoSFlag 0x18
#define sMQTTWillFlag 0x4

#define sMQTTConnReturnAccepted 0x0
#define sMQTTConnReturnUnacceptableProtocolVersion 0x1
#define sMQTTConnReturnIdentifierRejected 0x2
#define sMQTTConnReturnServerUnavailable 0x3
#define sMQTTConnReturnBadUsernameOrPassword 0x4

//!\brief Main Client class
class sMQTTClient
{
public:
	sMQTTClient(sMQTTBroker *parent, TCPClient &client);
	~sMQTTClient();

	void update();

	// Flush the underlying TCPClient socket
	void flushSocket() { _client.flush(); }

	//! check connection
	bool isConnected();
	void write(const char *buf, size_t length);

	//! get client id
	const std::string &getClientId()
	{
		return clientId;
	};

	std::map<uint16_t, sMQTTMessage> inflight;

	struct PendingQoS2
	{
		std::string topic;
		std::string payload;
		bool retain;
		unsigned char qos;
		unsigned long storedAt;
	};
	std::unordered_map<uint16_t, PendingQoS2> pendingQos2;

	// QoS2 housekeeping parameters
#ifndef SMQTT_DEFAULT_QOS2_TIMEOUT_MS
#define SMQTT_DEFAULT_QOS2_TIMEOUT_MS 15000UL
#endif
	static constexpr size_t PENDING_QOS2_MAX = 10; // max simultaneous QoS2 in-flight per client

	// Runtime configurable QoS2 timeout (initialized to macro default)
	void setQoS2Timeout(unsigned long ms) { qos2TimeoutMs = ms; }
	unsigned long getQoS2Timeout() const { return qos2TimeoutMs; }

private:
	void processMessage();
	void updateLiveStatus();

	char mqtt_flags;
	bool mqtt_connected;
	std::string clientId;
	unsigned short keepAlive;
	unsigned long aliveMillis;

	sMQTTBroker *_parent;
	TCPClient _client;
	sMQTTMessage message;
	unsigned long qos2TimeoutMs = SMQTT_DEFAULT_QOS2_TIMEOUT_MS;
};

typedef std::vector<sMQTTClient *> sMQTTClientList;
#endif
