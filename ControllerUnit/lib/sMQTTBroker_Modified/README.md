# sMQTTBroker_Modified

A lightweight, embeddable MQTT broker and client library for Arduino/ESP32/ESP8266 platforms, based on the original sMQTTBroker.

## Features

- MQTT 3.1/3.1.1 protocol support (CONNECT, PUBLISH, SUBSCRIBE, etc.)
- QoS 0, 1, and minimal stateless QoS 2 handshake support
- Retained messages and topic matching
- Inflight message tracking and retransmission for QoS 1
- Event-driven architecture for custom broker logic
- Designed for resource-constrained embedded systems

## Modifications in This Fork

- **Protocol Robustness:**
  - Added socket flush on new client connection to prevent protocol desync from stale TCP data.
  - Improved message ID extraction and duplicate detection for QoS 2.
  - Hardened minimal stateless QoS 2 handshake (PUBREC, PUBREL, PUBCOMP) for interoperability.
- **Reliability:**
  - Enhanced keepalive and connection timeout handling.
  - Improved inflight message retry logic for QoS 1.
- **Extensibility:**
  - Event system allows user code to handle publish, subscribe, and disconnect events.
  - Example user broker (`sMQTTBroker_User`) provided for easy customization.
- **Code Hygiene:**
  - Refactored for clarity, maintainability, and Arduino compatibility.
  - Added detailed debug logging for protocol events and errors.

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

### File-specific Modifications

- **sMQTTBroker.cpp/h**

  - Added socket flush on new client connection to prevent protocol desync.
  - Improved keepalive and connection timeout logic.
  - Enhanced inflight QoS1 message retry and PUBACK handling.
  - Hardened publish/subscribe event hooks for extensibility.

- **sMQTTClient.cpp/h**

  - Implemented minimal stateless QoS 2 handshake (PUBREC, PUBREL, PUBCOMP).
  - Added duplicate detection for QoS 2 message IDs.
  - Improved message ID extraction and protocol parsing.
  - Added debug logging for protocol events.

- **sMQTTMessage.cpp/h**

  - Improved message parsing robustness.
  - Hardened message ID and payload extraction.

- **sMQTTEvent.cpp/h**

  - Refactored event system for easier user extension.
  - Added new event types for publish, subscribe, and disconnect.

- **sMQTTTopic.cpp/h**

  - Improved retained message and topic matching logic.
  - Enhanced subscription management.

- **sMQTTplatform.h**

  - Platform abstraction for Arduino/ESP32/ESP8266 compatibility.

- **example.cpp**
  - Added minimal broker example with Serial logging and event handling.

## How to Use

1. Copy the folder to your PlatformIO or Arduino `lib/` directory.
2. Include `sMQTTBroker.h` and derive your own broker class from `sMQTTBroker`.
3. Implement the `onEvent` method to handle publish/subscribe events.
4. Call `broker.init(port)` in `setup()` and `broker.update()` in `loop()`.
