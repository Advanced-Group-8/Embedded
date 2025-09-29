#include "sMQTTMessage.h"
#include "sMQTTBroker.h"

sMQTTMessage::sMQTTMessage()
{
	reset();
	timestamp = 0;
};

sMQTTMessage::sMQTTMessage(Type t, unsigned char bits_d3_d0)
{
	create(t);
	buffer[0] |= bits_d3_d0;
	timestamp = 0;
};

void sMQTTMessage::getString(const char *&buff, unsigned short &len)
{
	len = (buff[0] << 8) | (buff[1]);
	buff += 2;
};

void sMQTTMessage::reset()
{
	state = FixedHeader;
	buffer.clear();
	size = 0;
	timestamp = 0;
};

void sMQTTMessage::incoming(char in_byte)
{
	buffer.push_back(in_byte);
	switch (state)
	{
	case FixedHeader:
		size = 0;
		multiplyer = 1;
		state = Length;
		break;
	case Length:
	{
		unsigned char encoded = in_byte & 0x7f;
		size += encoded * multiplyer;
		multiplyer *= 0x80;
		if ((in_byte & 0x80) == 0)
		{
			vheader = buffer.size();
			if (size == 0)
				state = Complete;
			else
			{
				buffer.reserve(size);
				state = VariableHeader;
			}
		}
	}
	break;
	case VariableHeader:
	case PayLoad:
		--size;
		if (size == 0)
		{
			state = Complete;
		}
		break;
	case Create:
		size++;
		break;
	case Complete:
	default:
		reset();
		break;
	}
};

int sMQTTMessage::encodeLength(char *msb, int length) const
{
	int count = 0;
	do
	{
		unsigned char encoded(length & 0x7f);
		length >>= 7;
		if (length)
			encoded |= 0x80;
		*msb = encoded;
		msb++;
		count++;
	} while (length);
	return count;
};

sMQTTError sMQTTMessage::sendTo(sMQTTClient *client, bool needRecalc)
{
	if (buffer.size())
	{
		if (needRecalc)
		{
			char *bytes = (char *)buffer.data();
			char size[4];
			size[0] = bytes[1];
			if (encodeLength(size, buffer.size() - vheader) > 1)
			{
				buffer.insert(buffer.begin() + 2, size[1]);
			}
			buffer[1] = size[0];
		}
		client->write(buffer.data(), buffer.size());
#if defined(ESP8266) || defined(ESP32)
		timestamp = millis();
#else
		timestamp = 0;
#endif
	}
	else
	{
		return sMQTTInvalidMessage;
	}
	return sMQTTOk;
};

// === QoS1 helper ===
uint16_t sMQTTMessage::getMessageId() const
{
	if (QoS() == 0)
		return 0;
	const char *ptr = getVHeader();
	unsigned short topicLen;
	getString(ptr, topicLen);
	ptr += topicLen;
	return ((uint8_t)ptr[0] << 8) | (uint8_t)ptr[1];
}
