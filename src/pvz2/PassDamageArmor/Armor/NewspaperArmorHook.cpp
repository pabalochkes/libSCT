#include "../PassDamageArmor.h"
#include "../Internal.h"
#include "../Offsets/Armor.h"
#include "../Offsets/NewspaperArmor.h"
#include "helpers/logger.h"
#include "memory/memory.h"
#include <dobby.h>
#include <cstring>
#include <cstdint>

using NewspaperArmorAllocator_t = void* (*)();
static NewspaperArmorAllocator_t RealNewspaperArmorInstanceAllocator = nullptr;

static void** g_PatchedNewspaperArmorVtable = nullptr;
static bool g_NewspaperArmorVtableBroken = false;

static void** GetPatchedNewspaperArmorVtable()
{
    if (g_NewspaperArmorVtableBroken)
    {
        return nullptr;
    }
    if (g_PatchedNewspaperArmorVtable)
    {
        return g_PatchedNewspaperArmorVtable;
    }

    auto realVtable = reinterpret_cast<void**>(Memory::RVA(Offsets::NewspaperArmor_Vtable));

    void* candidate[Offsets::Armor_Vtable_SlotCount];
    std::memcpy(candidate, realVtable, Offsets::Armor_Vtable_SlotCount * sizeof(void*));

    void* realSlotHere = candidate[Offsets::Armor_Vtable_ApplyDamageSlot / 8];

    if (!RealApplyDamage)
    {
        return nullptr;
    }

    if (reinterpret_cast<void*>(RealApplyDamage) != realSlotHere)
    {
        LOGE("GetPatchedNewspaperArmorVtable: real ApplyDamage slot (%p) does not match generic "
             "Armor's (%p) - refusing to install this patch on any NewspaperArmor instance",
             realSlotHere, (void*)RealApplyDamage);
        g_NewspaperArmorVtableBroken = true;
        return nullptr;
    }

    g_PatchedNewspaperArmorVtable = new void*[Offsets::Armor_Vtable_SlotCount];
    std::memcpy(g_PatchedNewspaperArmorVtable, candidate, Offsets::Armor_Vtable_SlotCount * sizeof(void*));
    g_PatchedNewspaperArmorVtable[Offsets::Armor_Vtable_ApplyDamageSlot / 8] =
        reinterpret_cast<void*>(Hooked_ApplyDamage);

    return g_PatchedNewspaperArmorVtable;
}

static void* Hooked_NewspaperArmorInstanceAllocator()
{
    void* obj = RealNewspaperArmorInstanceAllocator();
    if (obj)
    {
        void** patched = GetPatchedNewspaperArmorVtable();
        if (patched)
        {
            *reinterpret_cast<void***>(obj) = patched;
        }
    }
    return obj;
}

bool InstallNewspaperArmorApplyDamageHook()
{
    void* target = reinterpret_cast<void*>(Memory::RVA(Offsets::NewspaperArmor_InstanceAllocator));

    int result = DobbyHook(
        target,
        reinterpret_cast<void*>(Hooked_NewspaperArmorInstanceAllocator),
        reinterpret_cast<void**>(&RealNewspaperArmorInstanceAllocator)
    );

    if (result != 0)
    {
        LOGE("InstallNewspaperArmorApplyDamageHook: DobbyHook on NewspaperArmor instance allocator "
             "failed, code=%d", result);
        return false;
    }

    LOGI("InstallNewspaperArmorApplyDamageHook: hooked NewspaperArmor instance allocator @ %p", target);
    return true;
}
