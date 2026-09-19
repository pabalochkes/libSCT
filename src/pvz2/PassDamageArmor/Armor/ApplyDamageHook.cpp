#include "../PassDamageArmor.h"
#include "../Internal.h"
#include "../Offsets/Armor.h"
#include "../Offsets/ArmorPropertySheet.h"
#include "../Common/SafeMemory.h"
#include "helpers/logger.h"
#include "memory/memory.h"
#include "memory/abortrecovery.h"
#include <dobby.h>
#include <cstdint>
#include <cstring>

ApplyDamage_t RealApplyDamage = nullptr;

ApplyDamageResult Hooked_ApplyDamage(void* armor, void* hitInfo, uint64_t x2)
{
    bool matched = false;
    int64_t matchIndex = -1;

    void* propSheet = nullptr;
    RECOVER_FROM_ABORT(
        using ResolvePropSheet_t = void* (*)(void*);
        auto resolvePropSheet =
            reinterpret_cast<ResolvePropSheet_t>(Memory::RVA(Offsets::Armor_ResolvePropertySheet));
        propSheet = resolvePropSheet(
            reinterpret_cast<uint8_t*>(armor) + ArmorLayout::PropertySheetPtr);
    );

    if (propSheet)
    {
        uint64_t passDamageBegin = 0, passDamageEnd = 0;
        bool beginOk = SafeMem::ReadU64(
            reinterpret_cast<uint8_t*>(propSheet) + Layout::PassDamageFlags, passDamageBegin);
        bool endOk = SafeMem::ReadU64(
            reinterpret_cast<uint8_t*>(propSheet) + Layout::PassDamageFlags + 8, passDamageEnd);

        if (beginOk && endOk && passDamageEnd >= passDamageBegin)
        {
            size_t passDamageCount = (passDamageEnd - passDamageBegin) / 8;

            if (passDamageCount > 0 && passDamageCount < 64 && g_currentProjectile != nullptr)
            {
                uint64_t projectilePackedRef = 0;
                bool projRefOk = SafeMem::ReadU64(
                    reinterpret_cast<uint8_t*>(g_currentProjectile) + 0x60, projectilePackedRef);

                if (projRefOk && projectilePackedRef != 0)
                {
                    auto* entries = reinterpret_cast<uint8_t*>(passDamageBegin);
                    for (size_t i = 0; i < passDamageCount; i++)
                    {
                        uint64_t entryRaw = 0;
                        if (!SafeMem::ReadU64(entries + i * 8, entryRaw))
                            continue;

                        if (entryRaw == projectilePackedRef)
                        {
                            matched = true;
                            matchIndex = static_cast<int64_t>(i);
                            break;
                        }
                    }
                }
            }
        }
    }

    float incomingDamage = 0.0f;
    bool incomingDamageOk = false;

    if (matched)
    {
        incomingDamageOk = SafeMem::ReadMemory(
            reinterpret_cast<uint8_t*>(hitInfo) + 0x8, &incomingDamage, sizeof(incomingDamage));
    }

    ApplyDamageResult result = RealApplyDamage(armor, hitInfo, x2);

    if (matched && incomingDamageOk)
    {
        *reinterpret_cast<float*>(&result._opaque[0x8]) = incomingDamage;
        LOGI("Hooked_ApplyDamage: PassDamageFlags[%lld] matched - %.1f damage passed through",
             (long long)matchIndex, incomingDamage);
    }

    return result;
}

using ArmorAllocator_t = void* (*)();
static ArmorAllocator_t RealArmorInstanceAllocator = nullptr;
static void** g_PatchedArmorVtable = nullptr;

static void** GetPatchedArmorVtable()
{
    if (g_PatchedArmorVtable)
    {
        return g_PatchedArmorVtable;
    }

    auto realVtable = reinterpret_cast<void**>(Memory::RVA(Offsets::Armor_Vtable));

    g_PatchedArmorVtable = new void*[Offsets::Armor_Vtable_SlotCount];
    std::memcpy(g_PatchedArmorVtable, realVtable, Offsets::Armor_Vtable_SlotCount * sizeof(void*));

    RealApplyDamage = reinterpret_cast<ApplyDamage_t>(
        g_PatchedArmorVtable[Offsets::Armor_Vtable_ApplyDamageSlot / 8]);
    g_PatchedArmorVtable[Offsets::Armor_Vtable_ApplyDamageSlot / 8] =
        reinterpret_cast<void*>(Hooked_ApplyDamage);

    return g_PatchedArmorVtable;
}

static void* Hooked_ArmorInstanceAllocator()
{
    void* obj = RealArmorInstanceAllocator();
    if (obj)
    {
        *reinterpret_cast<void***>(obj) = GetPatchedArmorVtable();
    }
    return obj;
}

bool InstallApplyDamageHook()
{
    void* target = reinterpret_cast<void*>(Memory::RVA(Offsets::Armor_InstanceAllocator));

    int result = DobbyHook(
        target,
        reinterpret_cast<void*>(Hooked_ArmorInstanceAllocator),
        reinterpret_cast<void**>(&RealArmorInstanceAllocator)
    );

    if (result != 0)
    {
        LOGE("InstallApplyDamageHook: DobbyHook on Armor instance allocator failed, code=%d", result);
        return false;
    }

    LOGI("InstallApplyDamageHook: hooked Armor instance allocator @ %p", target);
    return true;
}
