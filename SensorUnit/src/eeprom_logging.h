#ifndef EEPROM_LOGGING_H
#define EEPROM_LOGGING_H

#include <Arduino.h>
#include <EEPROM.h>

// Minimal layout for JSON-queue
namespace Elog
{
    constexpr size_t BASE        = 0;      // Start of the EEPROM
    constexpr size_t RECORD_SIZE = 256;    // Bytes per JSON-post
    constexpr size_t META_SIZE   = 10;     // Reserved space for "Read-index", "Write-index" and "count"
    constexpr size_t EEPROM_SIZE = 8192;   // Reserved total bytes (Currently all)

    // Meta field addresses
    constexpr size_t READ_INDEX_ADDRESS  = BASE + 0;
    constexpr size_t WRITE_INDEX_ADDRESS = BASE + 2;
    constexpr size_t COUNT_ADDRESS       = BASE + 4;
    constexpr size_t SEQ_ADDRESS         = BASE + 6;

    // Data region
    constexpr size_t DATA_BASE = BASE + META_SIZE;
    constexpr size_t CAPACITY  = (EEPROM_SIZE - META_SIZE) / RECORD_SIZE;
    constexpr size_t MAXJSON_CHARS = RECORD_SIZE - 1;

    //Functions for handling the post-queue
    uint32_t getAndIncrementSequence();
    void enqueuePayload(const char payload[RECORD_SIZE]);

    // Fucntion that writes a json-payload to the EEPROM
    void writeToEeprom(const char payload[RECORD_SIZE], uint16_t index);
    void readFromEeprom(uint16_t index, char out[RECORD_SIZE]);

    //Debug getters
    uint16_t getReadIndex();
    uint16_t getWriteIndex();
    uint16_t getQueueCount();

    // Compile-time sanity checks
    static_assert(META_SIZE == 10, "META_SIZE must be exactly 10 bytes");
    static_assert(CAPACITY > 0, "EEPROM_SIZE too small for at least 1 record");
}

#endif