#pragma once
#include <cstdint>
#include <cstring>
#include <new>

struct SSOString
{
    uint8_t bytes[24] = {0};
    void* heapBuffer = nullptr;

    explicit SSOString(const char* s)
    {
        size_t len = std::strlen(s);
        if (len <= 22)
        {
            bytes[0] = static_cast<uint8_t>(len << 1);
            std::memcpy(bytes + 1, s, len);
            bytes[1 + len] = 0;
        }
        else
        {
            size_t needed = len + 1;
            size_t bufSize = (needed > 32) ? needed : 32;

            heapBuffer = ::operator new(bufSize);
            std::memcpy(heapBuffer, s, len);
            static_cast<char*>(heapBuffer)[len] = 0;
            uint64_t capacityWithFlag = (uint64_t)bufSize | 1ULL;
            uint64_t size = len;
            uint64_t dataPtr = reinterpret_cast<uint64_t>(heapBuffer);
            std::memcpy(bytes + 0,  &capacityWithFlag, 8);
            std::memcpy(bytes + 8,  &size,             8);
            std::memcpy(bytes + 16, &dataPtr,           8);
        }
    }
    ~SSOString() { if (heapBuffer) ::operator delete(heapBuffer); }
    void* ptr() { return bytes; }
};
