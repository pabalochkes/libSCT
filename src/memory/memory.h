#pragma once

#include <cstdint>

namespace Memory
{
    bool Initialize();

    uintptr_t GetBase();

    inline uintptr_t RVA(uintptr_t offset)
    {
        return GetBase() + offset;
    }
}