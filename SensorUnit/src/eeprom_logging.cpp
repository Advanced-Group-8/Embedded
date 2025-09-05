#include "eeprom_logging.h"

namespace Elog
{
    //Variables for handling log-queue
    static uint16_t readIndex  = 0;
    static uint16_t writeIndex = 0;
    static uint16_t queueCount = 0;

    // Local helper: calculate the starting address for a slot
    static inline size_t slotAddress(uint16_t index)
    {
        return DATA_BASE + (static_cast<size_t>(index) % CAPACITY) * RECORD_SIZE;
    }

    static uint32_t readSequence()
    {
        uint32_t value = 0;
        value |=(uint32_t)EEPROM.read(SEQ_ADDRESS + 0);
        value |=(uint32_t)EEPROM.read(SEQ_ADDRESS + 1 ) << 8;
        value |=(uint32_t)EEPROM.read(SEQ_ADDRESS + 2 ) << 16;
        value |=(uint32_t)EEPROM.read(SEQ_ADDRESS + 3 ) << 24;
        return value;
    }

    static void writeSequence(uint32_t value)
    {
        EEPROM.update(SEQ_ADDRESS + 0, (uint8_t)(value & 0xFF));
        EEPROM.update(SEQ_ADDRESS + 1, (uint8_t)((value >> 8) & 0xFF));
        EEPROM.update(SEQ_ADDRESS + 2, (uint8_t)((value >> 16) & 0xFF));
        EEPROM.update(SEQ_ADDRESS + 3, (uint8_t)((value >> 24) & 0xFF));
    }

    uint32_t getAndIncrementSequence()
    {
        uint32_t current = readSequence();
        writeSequence(current + 1);
        return current;
    }

    void enqueuePayload(const char payload[RECORD_SIZE])
    {
        // Write payload at current "WriteIndex"-slot
        writeToEeprom(payload, writeIndex);

        // calculate new index/count
        uint16_t newWrite = (uint16_t)(writeIndex + 1);
        if(newWrite >= CAPACITY) newWrite = 0;

        uint16_t newRead = readIndex;
        uint16_t newCount = queueCount;

        if(newCount < CAPACITY)
        {
            newCount = static_cast<uint16_t>(newCount + 1);
        }
        else
        {
            // Queue full: overwrite oldest and move readIndex forward
            newRead = static_cast<uint16_t>(newRead + 1);
            if(newRead >= CAPACITY) newRead = 0;
        }

        // Save new meta-values to EEPROM (low byte, high byte)
        EEPROM.update(WRITE_INDEX_ADDRESS + 0, static_cast<uint8_t>(newWrite & 0xFF));
        EEPROM.update(WRITE_INDEX_ADDRESS + 1, static_cast<uint8_t>((newWrite >> 8) & 0xFF));

        EEPROM.update(COUNT_ADDRESS + 0, static_cast<uint8_t>(newCount & 0xFF));
        EEPROM.update(COUNT_ADDRESS + 1, static_cast<uint8_t>((newCount >> 8) & 0xFF));

        EEPROM.update(READ_INDEX_ADDRESS + 0, static_cast<uint8_t>(newRead & 0xFF));
        EEPROM.update(READ_INDEX_ADDRESS + 1, static_cast<uint8_t>((newRead >> 8) & 0xFF));

        // Update RAM-copies
        writeIndex = newWrite;
        readIndex  = newRead;
        queueCount = newCount; 
    }

    // Finds next free slot and writes the json-payload to the EEPROM
    void writeToEeprom(const char payload[RECORD_SIZE], uint16_t index)
    {
        const size_t base = slotAddress(index);
        for (size_t i = 0; i < RECORD_SIZE; i++)
        {
            EEPROM.update(base + i, payload[i]);
        }
    }

    // ONLY TEMPORARY FOR DEBUGGING!!!
    uint16_t getReadIndex()  { return readIndex;  }
    uint16_t getWriteIndex() { return writeIndex; }
    uint16_t getQueueCount() { return queueCount; }
    void readFromEeprom(uint16_t index, char out[RECORD_SIZE])
    {
        const size_t base = slotAddress(index);
        for(size_t i = 0; i < RECORD_SIZE; i++)
        {
            out[i] = EEPROM.read(base + i);
        }
        out[RECORD_SIZE - 1] = '\0'; // Make sure last char is '\0'
    }
}