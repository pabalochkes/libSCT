#pragma once
#include <cstdint>

namespace UmbrellaLeaf::Engine
{
    uint64_t SpawnPopAnimEffect(void* owner, const char* popAnimName, float ox, float oy, float oz, int layer);

    void*    ResolveEffect(uint64_t ref);
    void     EffectPlayLabel(void* effect, const char* label);
}
