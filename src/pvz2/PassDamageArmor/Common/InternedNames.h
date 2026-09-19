#pragma once
#include <cstdint>
#include <cstdio>
#include "SafeMemory.h"
#include "memory/memory.h"

namespace InternPool
{
    constexpr uintptr_t kPoolHeaderPtrRVA = 0x025998d0;

    inline bool DecodeInternedIndex(uint32_t index, char* outBuf, size_t bufSize)
    {
        if (index == 0 || outBuf == nullptr || bufSize == 0)
            return false;

        uint64_t poolHeader = 0;
        if (!SafeMem::ReadU64(reinterpret_cast<void*>(Memory::RVA(kPoolHeaderPtrRVA)), poolHeader))
            return false;
        if (poolHeader == 0)
            return false;

        uint64_t arrayPtr = 0;
        if (!SafeMem::ReadU64(reinterpret_cast<void*>(poolHeader + 0x18), arrayPtr))
            return false;
        if (arrayPtr == 0)
            return false;

        uint64_t entryPtr = 0;
        if (!SafeMem::ReadU64(
                reinterpret_cast<void*>(arrayPtr + static_cast<uint64_t>(index - 1) * 8), entryPtr))
            return false;
        if (entryPtr == 0)
            return false;

        uint32_t packedLenFlag = 0;
        if (!SafeMem::ReadMemory(reinterpret_cast<void*>(entryPtr + 0x18), &packedLenFlag, sizeof(packedLenFlag)))
            return false;

        bool isHeap = (packedLenFlag & 1) != 0;

        uint64_t textAddr = 0;
        if (!isHeap)
        {
            textAddr = entryPtr + 0x1c;
        }
        else
        {
            if (!SafeMem::ReadU64(reinterpret_cast<void*>(entryPtr + 0x28), textAddr))
                return false;
        }
        if (textAddr == 0)
            return false;

        size_t maxChars = bufSize - 1;
        size_t written = 0;
        for (size_t i = 0; i < maxChars; i++)
        {
            uint32_t wch = 0;
            if (!SafeMem::ReadMemory(
                    reinterpret_cast<uint8_t*>(textAddr) + i * 4, &wch, sizeof(wch)))
                break;
            if (wch == 0)
                break;
            outBuf[written++] = (wch < 128) ? static_cast<char>(wch) : '?';
        }
        outBuf[written] = '\0';
        return written > 0;
    }

    inline void DecodePackedValue(uint64_t packed, char* outBuf, size_t bufSize)
    {
        if (static_cast<int64_t>(packed) >= 0)
        {
            snprintf(outBuf, bufSize, "<not interned: 0x%llx>", (unsigned long long)packed);
            return;
        }

        uint32_t lowIndex = static_cast<uint32_t>(packed & 0x7fffffffULL);
        uint32_t highIndex = static_cast<uint32_t>((packed >> 31) & 0x7fffffffULL);

        char lowText[128] = {0};
        char highText[128] = {0};
        bool lowOk = DecodeInternedIndex(lowIndex, lowText, sizeof(lowText));
        bool highOk = DecodeInternedIndex(highIndex, highText, sizeof(highText));

        snprintf(outBuf, bufSize, "name='%s'(idx=%u,ok=%d) ns='%s'(idx=%u,ok=%d)",
                 lowOk ? lowText : "?", lowIndex, lowOk ? 1 : 0,
                 highOk ? highText : "?", highIndex, highOk ? 1 : 0);
    }
}
