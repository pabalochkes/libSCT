#pragma once
#include <cstdint>

namespace Offsets
{
    constexpr uintptr_t Armor_ResolvePropertySheet = 0x005E0AF4;

    constexpr uintptr_t Armor_Vtable = 0x0249C370;
    constexpr uint32_t  Armor_Vtable_ApplyDamageSlot = 0x68;
    constexpr uint32_t  Armor_Vtable_SlotCount = 0x20;

    constexpr uintptr_t Armor_InstanceAllocator = 0x01150CFC;
}

namespace ArmorLayout
{
    constexpr uint32_t PropertySheetPtr   = 0x18; // RtWeakPtr<ArmorPropertySheet>
    constexpr uint32_t OwnerZombiePtr     = 0x20; // RtWeakPtr<Zombie>
    constexpr uint32_t Health             = 0x28;
    constexpr uint32_t MaxHealth          = 0x2c;
    constexpr uint32_t DamageState        = 0x30;
    constexpr uint32_t Destroyed          = 0x34;
    constexpr uint32_t Scored             = 0x35;
    constexpr uint32_t ArmorFlagsOverride = 0x38;
}
