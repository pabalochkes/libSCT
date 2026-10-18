#pragma once
#include <cstdint>

namespace UmbrellaLeaf
{
    namespace Offsets
    {
        constexpr uintptr_t PlantFramework_TypeNodeGetter   = 0x007C6F38;
        constexpr uintptr_t PlantFramework_Vtable           = 0x0239BDA0;
        constexpr uint32_t  PlantFramework_Vtable_SlotCount = 128;
        constexpr uint32_t  UmbrellaFramework_ObjectSize    = 0x50;

        constexpr uint32_t PlantFramework_TickSlot          = 0x120;
        constexpr uint32_t PlantFramework_CanUsePlantFood   = 0x140;
        constexpr uint32_t PlantFramework_UsePlantFood      = 0x148;
        constexpr uint32_t PlantFramework_PlantFoodEnd      = 0x150;
        constexpr uint32_t PlantFramework_TriggerRectSlot   = 0x258;
    }

    namespace FrameworkLayout
    {
        constexpr uint32_t OwnerPlant = 0x08;
        constexpr uint32_t UserState  = 0x10;
    }

    namespace PlantState
    {
        constexpr int Ready     = 4;
        constexpr int PlantFood = 5;
    }

    namespace PlantLayout
    {
        constexpr uint32_t SelfRef   = 0x08;
        constexpr uint32_t PosX      = 0x18;
        constexpr uint32_t PosY      = 0x1C;
        constexpr uint32_t State     = 0x9C;
        constexpr uint32_t Health    = 0xA4;
        constexpr uint32_t Row       = 0xB8;
        constexpr uint32_t Col       = 0xBC;
        constexpr uint32_t RigRef    = 0x118;
        constexpr uint32_t Framework = 0x80;
        constexpr uint32_t PropsRef  = 0x138;

        constexpr uint32_t ReceiveDamageSlot = 0x118;
        constexpr uint32_t RenderLayerSlot   = 0x160;
    }
}
