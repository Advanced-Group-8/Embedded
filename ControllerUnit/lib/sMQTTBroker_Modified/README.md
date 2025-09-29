# sMQTTBroker_Modified

A lightweight, embeddable MQTT broker and client library for Arduino/ESP32/ESP8266 platforms, based on the original sMQTTBroker.

## Features

- MQTT 3.1/3.1.1 protocol support (CONNECT, PUBLISH, SUBSCRIBE, etc.)
- Full support for QoS 0 and QoS 1 (including inflight message tracking and retransmission)
- Minimal stateless handshake support for QoS 2 (responds to PUBREC, PUBREL, PUBCOMP to avoid client/broker desync, but does not provide full QoS 2 delivery guarantees)
- Retained messages and topic matching
- Event-driven architecture for custom broker logic
- Designed for resource-constrained embedded systems

## Modifications in This Fork

- Extended the broker for full QoS 1 support, including inflight message tracking and retransmission.
- Added minimal stateless QoS 2 handshake support: the broker responds to QoS 2 control packets (PUBREC, PUBREL, PUBCOMP) to maintain protocol compliance and prevent desynchronization, but does not implement persistent state or exactly-once delivery for QoS 2.
- Provided an example user broker (`sMQTTBroker_User`) for easy customization.

## Example Usage

See `example.cpp` for a minimal broker implementation that prints received topics and payloads to Serial and can be extended to forward data to a backend.

## File Structure

- `sMQTTBroker.cpp/h` - Core broker logic
- `sMQTTClient.cpp/h` - Client connection and protocol handling
- `sMQTTMessage.cpp/h` - MQTT message parsing and construction
- `sMQTTEvent.cpp/h` - Event system for broker hooks
- `sMQTTTopic.cpp/h` - Topic and subscription management
- `sMQTTplatform.h` - Platform abstraction
- `example.cpp` - Example broker application

## How to Use

1. Copy the folder to your PlatformIO or Arduino `lib/` directory.
2. Include `sMQTTBroker.h` and derive your own broker class from `sMQTTBroker`.
3. Implement the `onEvent` method to handle publish/subscribe events.
4. Call `broker.init(port)` in `setup()` and `broker.update()` in `loop()`.
