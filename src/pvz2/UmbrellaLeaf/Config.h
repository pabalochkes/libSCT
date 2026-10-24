#pragma once
#include <cstdint>
#include "helpers/logger.h"

namespace UmbrellaLeaf::Config
{
    constexpr bool kDebugLog = false;

    constexpr uintptr_t kModuleSize = 0x2800000;

    // ==== leaf effect ============================================================================================
    constexpr const char* kLeafPopAnim = "POPANIM_PLANT_UMBRELLALEAF";
    constexpr float       kLeafOffset[3] = { -2.0f, -14.0f, 0.0f };

    // The game orders everything by one render number (0..499999). Each tile is 16000 apart
    constexpr int kRenderLayersPerTile = 16000;
    constexpr int kMaxRenderLayer      = 499999;
    constexpr int kLeafExtraLayers = 8000;
    constexpr int kLeafLayer       = kRenderLayersPerTile + kLeafExtraLayers;

    constexpr int kTicksPerFrame = 2;

    // ==== plant food explosion =====================================================================================
    constexpr uint32_t kExplodeActionIndex = 1;
    constexpr int   kMaxExplodeTargets     = 64;
}

#define UL_DEBUG(...) do { if (::UmbrellaLeaf::Config::kDebugLog) LOGI(__VA_ARGS__); } while (0)
