// eeprom_logging.h
#ifndef EEPROM_LOGGING_H
#define EEPROM_LOGGING_H

#include <Arduino.h>
#include <EEPROM.h>

namespace Elog
{
    constexpr size_t BASE        = 0;
    constexpr size_t RECORD_SIZE = 256;
    constexpr size_t META_SIZE   = 10;
    constexpr size_t EEPROM_SIZE = 8192;

    // Meta field addresses (2+2+2+4 bytes = 10 bytes)
    constexpr size_t READ_INDEX_ADDRESS  = BASE + 0;
    constexpr size_t WRITE_INDEX_ADDRESS = BASE + 2;
    constexpr size_t COUNT_ADDRESS       = BASE + 4;
    constexpr size_t SEQ_ADDRESS         = BASE + 6;

    // Data region
    constexpr size_t DATA_BASE = BASE + META_SIZE;
    constexpr size_t CAPACITY  = (EEPROM_SIZE - META_SIZE) / RECORD_SIZE;

    // Per-record header
    constexpr uint8_t STATUS_EMPTY   = 0xFF;
    constexpr uint8_t STATUS_PENDING = 0x01;
    constexpr uint8_t STATUS_SENT    = 0x02;

    constexpr size_t REC_STATUS_OFF = 0;  // 1 byte
    constexpr size_t REC_LEN_OFF    = 1;  // 2 bytes (LE)
    constexpr size_t REC_DATA_OFF   = 3;  // payload starts here

    // IMPORTANT: data capacity must exclude record header and allow null terminator
    constexpr size_t MAXJSON_CHARS  = RECORD_SIZE - REC_DATA_OFF - 1;

    // API
    bool begin(); // initialize or recover from slots if meta is invalid
    uint32_t getAndIncrementSequence();

    bool enqueuePayload(const char *payload);
    bool hasPending();
    bool peekPending(char out[RECORD_SIZE]);
    bool popPending(char out[RECORD_SIZE]);
    void markCurrentAsSent();

    // RAW helpers for testing
    void writeToEeprom(const char payload[RECORD_SIZE], uint16_t index);
    void readFromEeprom(uint16_t index, char out[RECORD_SIZE]);

    // Debug getters
    uint16_t getReadIndex();
    uint16_t getWriteIndex();
    uint16_t getQueueCount();

    static_assert(META_SIZE == 10, "META_SIZE must be exactly 10 bytes");
    static_assert(CAPACITY > 0, "EEPROM_SIZE too small for at least 1 record");
}

#endif
