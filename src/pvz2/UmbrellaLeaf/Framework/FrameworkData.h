#pragma once
#include <cstdint>
#include "../Offsets/PlantFramework.h"

namespace UmbrellaLeaf
{
    struct FrameworkData
    {
        uint32_t attacking;
        uint32_t phase;
        int32_t  ticksLeft;
        uint32_t bound;
        uint32_t probe;
        uint32_t age;
        int32_t  pfRig;
        uint32_t pfDone;
        uint32_t started;
        uint64_t leafRef;
    };
    static_assert(sizeof(FrameworkData) <= Offsets::UmbrellaFramework_ObjectSize - FrameworkLayout::UserState,
                  "FrameworkData does not fit the framework object");

    inline FrameworkData& GetData(void* framework)
    {
        return *reinterpret_cast<FrameworkData*>(static_cast<uint8_t*>(framework) + FrameworkLayout::UserState);
    }
    inline void* FrameworkOf(const uint8_t* plant)
    {
        return *reinterpret_cast<void* const*>(plant + PlantLayout::Framework);
    }
}
