#pragma once
#include <cstdint>

namespace UmbrellaLeaf
{
    namespace RigState
    {
        constexpr int Idle          = 1;
        constexpr int Attack        = 2;
        constexpr int PlantFoodOn   = 3;
        constexpr int PlantFoodLoop = 4;
        constexpr int PlantFoodOff  = 5;
    }

    namespace RigLayout
    {
        constexpr uint32_t OnFinishedSlot    = 0xB0;
        constexpr uint32_t PlayIdleSlot      = 0x100;
        constexpr uint32_t VtableSlotCount   = 100;
        constexpr uint32_t State             = 0x24C;
        constexpr uint32_t UseActionDelegate = 0x2C8;
        constexpr uint32_t ObjectSize        = 0x428;
    }

    namespace ActionLayout
    {
        constexpr uint32_t PropsActionsBegin = 0x98;
        constexpr uint32_t PropsActionsEnd   = 0xA0;
        constexpr uint32_t Stride            = 0x78;
        constexpr uint32_t ExplodeType       = 0x10;
        constexpr uint32_t ExplodeRadius     = 0x14;
        constexpr uint32_t ExecuteSlot       = 0x80;
    }

    namespace ProjectileLayout
    {
        constexpr uint32_t SelfRef = 0x08;
        constexpr uint32_t TypeRef = 0x60;
        constexpr uint32_t PosX    = 0x18;
        constexpr uint32_t PosY    = 0x1C;
        constexpr uint32_t PosZ    = 0x20;
        constexpr uint32_t Row     = 0xB4;
    }

    namespace ProjectilePropsLayout
    {
        constexpr uint32_t BaseDamage = 0x40;
    }

    namespace PrimalLayout
    {
        constexpr uint32_t PropsSize         = 0x298;
        constexpr uint32_t StunDuration      = 0x280;
        constexpr uint32_t KnockbackChance   = 0x284;
        constexpr uint32_t KnockbackDistance = 0x288;
        constexpr uint32_t KnockbackTime     = 0x28C;
        constexpr uint32_t KnockbackHeight   = 0x290;
        constexpr uint32_t ProjectileProps   = 0x58;
        constexpr uint32_t ProjectileShooter = 0x68;
    }

    namespace DamageInfoLayout
    {
        constexpr uint32_t Size   = 0x80;
        constexpr uint32_t Source = 0x00;
        constexpr uint32_t Damage = 0x08;
    }
}
