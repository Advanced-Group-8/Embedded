// eeprom_logging.h
#ifndef EEPROM_LOGGING_H
#define EEPROM_LOGGING_H

#include <Arduino.h>
#include <EEPROM.h>

/**
 * @file eeprom_logging.h
 * @brief EEPROM-backed persistent queue for JSON payloads.
 *
 * This module provides a ring buffer stored in EEPROM to safely queue
 * unsent payloads across resets or power loss.
 * 
 * Layout overview:
 * [META (10B)] [RECORD 0 (256B)] [RECORD 1 (256B)] ...
 * 
 * META = 10 bytes total:
 *   READ_INDEX  (2B)
 *   WRITE_INDEX (2B)
 *   COUNT       (2B)
 *   SEQUENCE    (4B)
 * 
 * Each RECORD = 256 bytes total:
 *   STATUS  (1B)   : 0xFF = empty, 0x01 = pending, 0x02 = sent
 *   LENGTH  (2B)   : little endian payload length
 *   DATA[n] (<= 252B): JSON string + '\0'
 */

namespace Elog
{
    // =========================
    // Layout Configuration
    // =========================

    constexpr size_t BASE        = 0;       ///< EEPROM base address.
    constexpr size_t RECORD_SIZE = 256;     ///< Size of each record in bytes.
    constexpr size_t META_SIZE   = 10;      ///< 2+2+2+4 bytes of metadata.
    constexpr size_t EEPROM_SIZE = 800; //8192;    ///< Total EEPROM size in bytes.

    // --- Meta field offsets ---
    constexpr size_t READ_INDEX_ADDRESS  = BASE + 0;  ///< uint16_t read pointer.
    constexpr size_t WRITE_INDEX_ADDRESS = BASE + 2;  ///< uint16_t write pointer.
    constexpr size_t COUNT_ADDRESS       = BASE + 4;  ///< uint16_t active queue size.
    constexpr size_t SEQ_ADDRESS         = BASE + 6;  ///< uint32_t sequence counter.

    // --- Data region ---
    constexpr size_t DATA_BASE = BASE + META_SIZE; ///< First record starts here.
    constexpr size_t CAPACITY  = (EEPROM_SIZE - META_SIZE) / RECORD_SIZE; ///< Max records that fit.

    // =========================
    // Record Header Structure
    // =========================
    constexpr uint8_t STATUS_EMPTY   = 0xFF; ///< Slot unused or cleared.
    constexpr uint8_t STATUS_PENDING = 0x01; ///< Payload waiting to send.
    constexpr uint8_t STATUS_SENT    = 0x02; ///< Payload acknowledged.

    constexpr size_t REC_STATUS_OFF = 0; ///< Offset to status byte.
    constexpr size_t REC_LEN_OFF    = 1; ///< Offset to 2-byte length (LE).
    constexpr size_t REC_DATA_OFF   = 3; ///< Offset to payload start.

    /// Max JSON chars allowed in payload (excluding header + NUL).
    constexpr size_t MAXJSON_CHARS = RECORD_SIZE - REC_DATA_OFF - 1;

    // =========================
    // API FUNCTIONS
    // =========================

    /** 
     * @brief Initialize EEPROM metadata.
     * 
     * If invalid values are detected (e.g., after a fresh flash),
     * this function resets indices and clears metadata region.
     * 
     * @return true if initialization or recovery succeeded.
     */
    bool begin();

    /**
     * @brief Get current sequence number and increment.
     * 
     * Useful for uniquely tagging outgoing payloads.
     * 
     * @return Monotonic uint32_t sequence number.
     */
    uint32_t getAndIncrementSequence();

    /**
     * @brief Store a new JSON payload in EEPROM queue.
     * 
     * @param payload NUL-terminated string (max MAXJSON_CHARS).
     * @return true if enqueued successfully; false if queue full or payload too large.
     */
    bool enqueuePayload(const char *payload);

    /**
     * @brief Check if there are pending (unsent) payloads.
     */
    bool hasPending();

    /**
     * @brief Copy oldest pending record into out buffer without removing it.
     * 
     * @param out Output buffer of RECORD_SIZE bytes.
     * @return true if a pending record was found.
     */
    bool peekPending(char out[RECORD_SIZE]);

    /**
     * @brief Copy and remove oldest pending record.
     * 
     * Advances read index.
     * 
     * @param out Output buffer of RECORD_SIZE bytes.
     * @return true if a pending record was popped.
     */
    bool popPending(char out[RECORD_SIZE]);

    /**
     * @brief Mark most recently popped or peeked record as sent.
     * 
     * This clears the record slot for reuse.
     */
    void markCurrentAsSent();

    // =========================
    // RAW HELPERS (for testing)
    // =========================

    void writeToEeprom(const char payload[RECORD_SIZE], uint16_t index);
    void readFromEeprom(uint16_t index, char out[RECORD_SIZE]);

    // =========================
    // DEBUG INSPECTION
    // =========================
    uint16_t getReadIndex();
    uint16_t getWriteIndex();
    uint16_t getQueueCount();

    // =========================
    // BUILD-TIME CHECKS
    // =========================
    static_assert(META_SIZE == 10, "META_SIZE must be exactly 10 bytes");
    static_assert(CAPACITY > 0, "EEPROM_SIZE too small for at least 1 record");
}

#endif