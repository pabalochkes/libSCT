#include "../Internal.h"
#include "../Common/Reflection.h"
#include "../Common/SSOString.h"
#include "../Offsets/PlantPropertySheet.h"
#include "../Offsets/Registry.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include "memory/crashlog.h"
#include "memory/memory.h"
#include <cstdint>
#include <cstring>
#include <new>

namespace UmbrellaLeaf
{
    namespace
    {
        using GetBaseType_t       = void* (*)();
        using RegisterField_t     = void  (*)(void*, void*, void*, uint32_t, void*);
        using GetVectorType_t     = void* (*)(void*, void*);
        using MakeBasicType_t     = void* (*)(void*, int, int);
        using MakeContainerType_t = void* (*)(void*, int, void*, int);
        using RealCtor_t          = void  (*)(void*);

        RealCtor_t            RealPlantPropsCtor = nullptr;
        void**                g_ClonedVtable     = nullptr;
        Reflect::TypeNodeSlot g_slot;

        void* Hooked_GetOwnTypeNode()
        {
            return Reflect::HookedTypeNode(g_slot, __builtin_return_address(0));
        }

        void* UmbrellaProps_Allocator()
        {
            CrashLog::Install();
            void* obj = ::operator new(UmbrellaLayout::ExtendedObjectSize);
            RealPlantPropsCtor(obj);
            std::memset(reinterpret_cast<uint8_t*>(obj) + UmbrellaLayout::BaseObjectSize, 0,
                        UmbrellaLayout::ExtraFieldSize);

            if (!g_ClonedVtable)
            {
                void** realVtable = *reinterpret_cast<void***>(obj);
                g_ClonedVtable = new void*[UmbrellaLayout::VtableSlotCount];
                std::memcpy(g_ClonedVtable, realVtable, UmbrellaLayout::VtableSlotCount * sizeof(void*));
                g_slot.realGetter = reinterpret_cast<void* (*)()>(realVtable[0]);
                g_ClonedVtable[0] = reinterpret_cast<void*>(Hooked_GetOwnTypeNode);
                LOGI("UmbrellaProps: vtable cloned");
            }
            *reinterpret_cast<void***>(obj) = g_ClonedVtable;
            LOGI("UmbrellaProps: allocated %p", obj);
            return obj;
        }

        void UmbrellaProps_FieldRegistrar(void* ctx, void* builder)
        {
            Reflect::BindBuilder(g_slot, builder);
            Reflect::SetBaseByName(ctx, builder, "PlantPropertySheet");

            auto vtbl          = *reinterpret_cast<void***>(ctx);
            auto registerField = reinterpret_cast<RegisterField_t>(vtbl[0x68 / 8]);
            auto makeBasic     = reinterpret_cast<MakeBasicType_t>(vtbl[0x38 / 8]);
            auto makeContainer = reinterpret_cast<MakeContainerType_t>(vtbl[0x50 / 8]);

            auto addField = [&](const char* name, uint32_t offset, void* type)
            {
                SSOString n(name);
                registerField(ctx, builder, n.ptr(), offset, type);
            };

            // BounceableProjectiles: same vector type as PassDamageFlags
            auto getProjectileRefVectorType = reinterpret_cast<GetVectorType_t>(
                Memory::RVA(Offsets::GetProjectileRefVectorType));
            SSOString vecTypeName("std::vector<RtWeakPtr<ProjectilePropertySheet>>");
            if (void* projVecType = getProjectileRefVectorType(ctx, vecTypeName.ptr()))
                addField("BounceableProjectiles", UmbrellaLayout::BounceableProjectiles, projVecType);
            else
                LOGE("UmbrellaProps_FieldRegistrar: projectile vector type is null");

            void* stringType = makeContainer(ctx, 0, makeBasic(ctx, 4, 1), 0);
            void* intType    = makeBasic(ctx, 6, 4);
            void* floatType  = makeBasic(ctx, 8, 4);

            addField("AudioEventName",           UmbrellaLayout::AudioEventName,           stringType);
            addField("ProjectileBounceDistance", UmbrellaLayout::ProjectileBounceDistance, intType);
            addField("ProjectileBounceHeight",   UmbrellaLayout::ProjectileBounceHeight,   intType);
            addField("ProjectileBounceTime",     UmbrellaLayout::ProjectileBounceTime,     floatType);
            addField("StunPFDuration",           UmbrellaLayout::StunPFDuration,           floatType);
            addField("KnockbackPFTime",          UmbrellaLayout::KnockbackPFTime,          floatType);
            addField("KnockbackPFDistance",      UmbrellaLayout::KnockbackPFDistance,      floatType);
            addField("KnockbackPFHeight",        UmbrellaLayout::KnockbackPFHeight,        floatType);
            addField("KnockbackPFChance",        UmbrellaLayout::KnockbackPFChance,        floatType);
            addField("DamageMultiplier",         UmbrellaLayout::DamageMultiplier,         floatType);
        }
    }

    void** GetPropsVtable() { return g_ClonedVtable; }

    bool RegisterPropertySheet()
    {
        RealPlantPropsCtor = reinterpret_cast<RealCtor_t>(Memory::RVA(Offsets::PlantPropertySheet_Ctor));

        g_slot.typeNode = Reflect::NewTypeNode();
        if (!g_slot.typeNode) { LOGE("RegisterPropertySheet: type node build failed"); return false; }

        void* baseType = nullptr;
        RECOVER_FROM_ABORT(
            baseType = reinterpret_cast<GetBaseType_t>(Memory::RVA(Offsets::PlantPropertySheet_BaseTypeSingleton))();
        );
        if (!baseType) { LOGE("RegisterPropertySheet: PlantPropertySheet base type null"); return false; }

        if (!Reflect::RegisterClass(g_slot.typeNode, "UmbrellaProps", baseType, (void*)UmbrellaProps_Allocator,
                                    (void*)UmbrellaProps_FieldRegistrar, UmbrellaLayout::ExtendedObjectSize))
            return false;

        Reflect::RegisterObjClassFactory("UmbrellaProps", baseType, (void*)UmbrellaProps_Allocator);

        LOGI("RegisterPropertySheet: UmbrellaProps registered");
        return true;
    }
}
