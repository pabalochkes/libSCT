#pragma once
#include <cstdint>

namespace Offsets
{
    constexpr uintptr_t ArmorPropertySheet_Ctor = 0x0073A6A8;

    constexpr uintptr_t GetPropertyRegistry     = 0x01624024;
    constexpr uintptr_t FindPropertyRegistry    = 0x0162413C;
    constexpr uintptr_t RegisterObjClassFactory = 0x0163AC20;

    constexpr uintptr_t CtorHelper = 0x0163A40C;

    constexpr uintptr_t ArmorPropertySheetBaseTypeSingleton = 0x00739EF0;

    constexpr uintptr_t PrepareBuilderAgainstRegistry = 0x0163B0DC;

    constexpr uintptr_t GetProjectileRefVectorType = 0x00C15D14;
}

namespace Layout
{
    constexpr uint32_t ClassName            = 0x28;
    constexpr uint32_t ArmorType            = 0x40;
    constexpr uint32_t BaseHealth           = 0x58;
    constexpr uint32_t ArmorLayers          = 0x60;
    constexpr uint32_t ArmorLayerHealth     = 0x78;
    constexpr uint32_t ParticleLayerOverride = 0x90;
    constexpr uint32_t ImpactSoundEvent     = 0xA8;
    constexpr uint32_t DropSoundEvent       = 0xC0;
    constexpr uint32_t ArmorFlags           = 0xD8;

    constexpr uint32_t BaseObjectSize       = 0xF0;
    constexpr uint32_t PassDamageFlags      = BaseObjectSize;
    constexpr uint32_t ExtraFieldSize       = 0x18;
    constexpr uint32_t ExtendedObjectSize   = BaseObjectSize + ExtraFieldSize;
}
