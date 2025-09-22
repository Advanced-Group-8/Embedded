#include "eeprom_logging.h"

namespace Elog
{
    static uint16_t readIndex  = 0;
    static uint16_t writeIndex = 0;
    static uint16_t queueCount = 0;

    static inline size_t slotAddress(uint16_t index)
    {
        return DATA_BASE + (static_cast<size_t>(index) % CAPACITY) * RECORD_SIZE;
    }

    // --- Meta helpers ---
    static uint16_t readU16(size_t address)
    {
        uint16_t lo = EEPROM.read(address + 0);
        uint16_t hi = EEPROM.read(address + 1);
        return static_cast<uint16_t>((hi << 8) | lo);
    }

    static void writeU16(size_t address, uint16_t v)
    {
        EEPROM.update(address + 0, static_cast<uint8_t>(v & 0xFF));
        EEPROM.update(address + 1, static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    static uint32_t readU32(size_t address)
    {
        uint32_t v = 0;
        v |= (uint32_t)EEPROM.read(address + 0);
        v |= (uint32_t)EEPROM.read(address + 1) << 8;
        v |= (uint32_t)EEPROM.read(address + 2) << 16;
        v |= (uint32_t)EEPROM.read(address + 3) << 24;
        return v;
    }

    static void writeU32(size_t address, uint32_t v)
    {
        EEPROM.update(address + 0, (uint8_t)(v & 0xFF));
        EEPROM.update(address + 1, (uint8_t)((v >> 8) & 0xFF));
        EEPROM.update(address + 2, (uint8_t)((v >> 16) & 0xFF));
        EEPROM.update(address + 3, (uint8_t)((v >> 24) & 0xFF));
    }

    // --- Sequence ---
    static uint32_t readSequence()
    {
        return readU32(SEQ_ADDRESS);
    }

    static void writeSequence(uint32_t value)
    {
        writeU32(SEQ_ADDRESS, value);
    }

    uint32_t getAndIncrementSequence()
    {
        uint32_t current = readSequence();
        writeSequence(current + 1);
        return current;
    }

    // --- Record helpers ---
    static uint8_t readStatus(uint16_t index)
    {
        return EEPROM.read(slotAddress(index) + REC_STATUS_OFF);
    }

    static void writeStatus(uint16_t index, uint8_t s)
    {
        EEPROM.update(slotAddress(index) + REC_STATUS_OFF, s);
    }

    static uint16_t readLen(uint16_t index)
    {
        size_t base = slotAddress(index);
        uint16_t lo = EEPROM.read(base + REC_LEN_OFF + 0);
        uint16_t hi = EEPROM.read(base + REC_LEN_OFF + 1);
        return static_cast<uint16_t>((hi << 8) | lo);
    }

    static void writeLen(uint16_t index, uint16_t len)
    {
        size_t base = slotAddress(index);
        EEPROM.update(base + REC_LEN_OFF + 0, (uint8_t)(len & 0xFF));
        EEPROM.update(base + REC_LEN_OFF + 1, (uint8_t)((len >> 8) & 0xFF));
    }

    // --- Public API ---
    bool begin()
    {
        // Read meta fields from EEPROM
        uint16_t ri = (uint16_t)(EEPROM.read(READ_INDEX_ADDRESS)  | (EEPROM.read(READ_INDEX_ADDRESS+1)  << 8));
        uint16_t wi = (uint16_t)(EEPROM.read(WRITE_INDEX_ADDRESS) | (EEPROM.read(WRITE_INDEX_ADDRESS+1) << 8));
        uint16_t qc = (uint16_t)(EEPROM.read(COUNT_ADDRESS)       | (EEPROM.read(COUNT_ADDRESS+1)       << 8));

        auto sane = [&](uint16_t r, uint16_t w, uint16_t c){
            return (r < CAPACITY) && (w < CAPACITY) && (c <= CAPACITY);
        };

        // Fast path: use meta if they look sane and head points to a PENDING when count>0
        if (sane(ri, wi, qc)) {
            readIndex  = ri;
            writeIndex = wi;
            queueCount = qc;

            if (queueCount == 0 || readStatus(readIndex) == STATUS_PENDING) {
                return false; // already initialized
            }
            // Otherwise fall through to full scan recovery
        }

        // --- Full scan to rebuild a contiguous PENDING run ---
        // Find HEAD: first PENDING that is preceded by a non-PENDING
        int32_t head = -1;
        for (uint16_t i = 0; i < CAPACITY; ++i)
        {
            uint8_t cur = EEPROM.read(slotAddress(i));
            if (cur != STATUS_PENDING) continue;

            uint16_t prev = (i == 0) ? (CAPACITY - 1) : (i - 1);
            uint8_t prevStatus = EEPROM.read(slotAddress(prev));

            if (prevStatus != STATUS_PENDING)
            {
                head = (int32_t)i;
                break;
            }
        }

        if (head == -1)
        {
            // No pending entries found → reset queue
            readIndex  = 0;
            writeIndex = 0;
            queueCount = 0;

            // Persist repaired meta
            writeU16(READ_INDEX_ADDRESS,  readIndex);
            writeU16(WRITE_INDEX_ADDRESS, writeIndex);
            writeU16(COUNT_ADDRESS,       queueCount);
            return true; // recovered from empty
        }

        // Find TAIL: iterate forward until the first non-PENDING
        uint16_t idx   = (uint16_t)head;
        uint16_t count = 0;
        while (count < CAPACITY && EEPROM.read(slotAddress(idx)) == STATUS_PENDING)
        {
            idx = (uint16_t)((idx + 1) % CAPACITY);
            ++count;
        }

        // Tail is last PENDING; writeIndex must be right after tail
        uint16_t tail = (uint16_t)((idx + CAPACITY - 1) % CAPACITY);

        readIndex  = (uint16_t)head;
        writeIndex = (uint16_t)((tail + 1) % CAPACITY);
        queueCount = count;

        // Persist repaired meta
        writeU16(READ_INDEX_ADDRESS,  readIndex);
        writeU16(WRITE_INDEX_ADDRESS, writeIndex);
        writeU16(COUNT_ADDRESS,       queueCount);

        return true; // recovered from slot scan
    }

    bool enqueuePayload(const char *payload)
    {
        if (!payload) return false;

        // Validate length so we never overflow the record area
        size_t len = strnlen(payload, MAXJSON_CHARS + 1);
        if (len == 0 || len > MAXJSON_CHARS) return false;

        // Write record header: PENDING + length
        size_t base = slotAddress(writeIndex);
        writeStatus(writeIndex, STATUS_PENDING);
        writeLen(writeIndex, static_cast<uint16_t>(len));

        // Write payload and null terminator
        for (size_t i = 0; i < len; ++i)
        {
            EEPROM.update(base + REC_DATA_OFF + i, (uint8_t)payload[i]);
        }
        EEPROM.update(base + REC_DATA_OFF + len, (uint8_t)'\0');

        // Advance indices (circular queue)
        uint16_t newWrite = (uint16_t)((writeIndex + 1) % CAPACITY);

        uint16_t newRead  = readIndex;
        uint16_t newCount = queueCount;

        if (newCount < CAPACITY)
        {
            newCount++;
        }
        else
        {
            // Queue full: overwrite the oldest and move readIndex forward by one
            // (We do not change count; it stays at CAPACITY.)
            uint16_t tmp = (uint16_t)(readIndex + 1);
            newRead = (tmp >= CAPACITY) ? 0 : tmp;
        }

        // Persist meta
        writeU16(WRITE_INDEX_ADDRESS, newWrite);
        writeU16(READ_INDEX_ADDRESS,  newRead);
        writeU16(COUNT_ADDRESS,       newCount);

        // Update RAM copies
        writeIndex = newWrite;
        readIndex  = newRead;
        queueCount = newCount;

        return true;
    }

    bool hasPending()
    {
        if (queueCount == 0) return false;

        // Probe forward from readIndex until we find a PENDING
        uint16_t index = readIndex;
        for (uint16_t i = 0; i < CAPACITY; ++i)
        {
            if (readStatus(index) == STATUS_PENDING) return true;
            index = (uint16_t)((index + 1) % CAPACITY);
        }
        return false;
    }

    bool peekPending(char out[RECORD_SIZE])
    {
        if (!out) return false;
        if (!hasPending()) return false;

        // Locate the next PENDING slot and copy its payload
        uint16_t index = readIndex;
        for (uint16_t i = 0; i < CAPACITY; ++i)
        {
            if (readStatus(index) == STATUS_PENDING)
            {
                size_t base = slotAddress(index);
                uint16_t len = readLen(index);
                if (len > MAXJSON_CHARS) len = MAXJSON_CHARS;

                for (uint16_t j = 0; j < len; ++j)
                {
                    out[j] = (char)EEPROM.read(base + REC_DATA_OFF + j);
                }
                out[len] = '\0';
                return true;
            }
            index = (uint16_t)((index + 1) % CAPACITY);
        }
        return false;
    }

    bool popPending(char out[RECORD_SIZE])
    {
        // Read the next pending entry and then mark it as SENT + advance readIndex
        if (!peekPending(out)) return false;
        markCurrentAsSent();
        return true;
    }

    void markCurrentAsSent()
    {
        // Walk forward from readIndex to find the next PENDING, mark it SENT,
        // then advance readIndex and decrement count.
        uint16_t index = readIndex;
        for (uint16_t i = 0; i < CAPACITY; ++i)
        {
            if (readStatus(index) == STATUS_PENDING)
            {
                writeStatus(index, STATUS_SENT);

                uint16_t newRead  = (uint16_t)((index + 1) % CAPACITY);
                uint16_t newCount = (queueCount > 0) ? (uint16_t)(queueCount - 1) : 0;

                writeU16(READ_INDEX_ADDRESS, newRead);
                writeU16(COUNT_ADDRESS,      newCount);

                readIndex  = newRead;
                queueCount = newCount;
                return;
            }
            index = (uint16_t)((index + 1) % CAPACITY);
        }
    }

    // RAW Write/Read. Not in use
    void writeToEeprom(const char payload[RECORD_SIZE], uint16_t index)
    {
        const size_t base = slotAddress(index);
        for (size_t i = 0; i < RECORD_SIZE; i++)
        {
            EEPROM.update(base + i, payload[i]);
        }
    }

    void readFromEeprom(uint16_t index, char out[RECORD_SIZE])
    {
        const size_t base = slotAddress(index);
        for (size_t i = 0; i < RECORD_SIZE; i++)
        {
            out[i] = EEPROM.read(base + i);
        }
        out[RECORD_SIZE - 1] = '\0';
    }

    uint16_t getReadIndex()  { return readIndex;  }
    uint16_t getWriteIndex() { return writeIndex; }
    uint16_t getQueueCount() { return queueCount; }
}