#pragma once
#include <cstdint>
#include "../Engine/EngineApi.h"

namespace UmbrellaLeaf::Scan
{
    bool Run(void* owner, uint8_t* plant, const Engine::Rect& trigger);
}
