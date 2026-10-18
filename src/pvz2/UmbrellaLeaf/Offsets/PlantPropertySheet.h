#pragma once
#include <cstdint>

namespace UmbrellaLeaf
{
    namespace Offsets
    {
        constexpr uintptr_t PlantPropertySheet_Ctor              = 0x00611410;
        constexpr uintptr_t PlantPropertySheet_BaseTypeSingleton = 0x0060FCD0;
    }

    namespace UmbrellaLayout
    {
        constexpr uint32_t BaseObjectSize           = 0x1C8;

        constexpr uint32_t BounceableProjectiles    = 0x1C8;
        constexpr uint32_t AudioEventName           = 0x1E0;
        constexpr uint32_t ProjectileBounceDistance = 0x1F8;
        constexpr uint32_t ProjectileBounceHeight   = 0x1FC;
        constexpr uint32_t ProjectileBounceTime     = 0x200;
        constexpr uint32_t StunPFDuration           = 0x204;
        constexpr uint32_t KnockbackPFTime          = 0x208;
        constexpr uint32_t KnockbackPFDistance      = 0x20C;
        constexpr uint32_t KnockbackPFHeight        = 0x210;
        constexpr uint32_t KnockbackPFChance        = 0x214;
        constexpr uint32_t DamageMultiplier         = 0x218;

        constexpr uint32_t ExtendedObjectSize       = 0x220;
        constexpr uint32_t ExtraFieldSize           = ExtendedObjectSize - BaseObjectSize;
        constexpr uint32_t VtableSlotCount          = 96;
    }
}
