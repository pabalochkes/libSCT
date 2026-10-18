#pragma once
#include <cstdint>
#include "../Engine/EngineApi.h"

namespace UmbrellaLeaf::Bounce
{
    struct Context
    {
        void*           owner;
        const uint8_t*  plant;
        const uint8_t*  props;
        const uint64_t* listBegin;
        const uint64_t* listEnd;
        Engine::Rect    trigger;
    };

    bool ReadBounceList(const uint8_t* props, const uint64_t*& begin, const uint64_t*& end);

    void Tick();
    bool TryQueue(uint8_t* proj, const Context& ctx);
    int  FlushPending(void* owner);
}
