#include "../PassDamageArmor.h"
#include "../Offsets/ArmorPropertySheet.h"
#include "../Common/SSOString.h"
#include "helpers/logger.h"
#include "memory/memory.h"
#include "memory/abortrecovery.h"
#include <cstring>
#include <cstdint>
#include <new>

using RealCtor_t       = void  (*)(void*);
using CtorHelper_t     = void  (*)(void*);
using GetSingleton_t   = void* (*)();
using FindInRegistry_t = void* (*)(void*);
using GetArmorBaseType_t = void* (*)();
using RegisterType_t   = void* (*)(void*, const char*, void*, void*);
using RegisterClass_t  = void  (*)(void*, const char*, void*, uint32_t, uint32_t);
using FindTypeByName_t = void* (*)(void*, void*);
using SetBaseType_t    = void  (*)(void*, void*, void*, int);
using RegisterField_t  = void  (*)(void*, void*, void*, uint32_t, void*);
using GetVectorType_t  = void* (*)(void*, void*);

static RealCtor_t RealArmorCtor = nullptr;

static void* g_TypeRegistry = nullptr;
static void** g_ClonedPropSheetVtable = nullptr;
constexpr uint32_t PROP_SHEET_VTABLE_SLOT_COUNT = 48;
static void* g_LastBuilder = nullptr;

using GetOwnTypeNode_t = void* (*)();
static GetOwnTypeNode_t RealGetTypeNode = nullptr;
static void* g_OwnTypeNodeWrapper = nullptr;

static void* Hooked_GetOwnTypeNode()
{
    void* callerAbs = __builtin_return_address(0);
    uintptr_t callerGhidraAddr = reinterpret_cast<uintptr_t>(callerAbs) - Memory::GetBase() + 0x100000;

    constexpr uintptr_t kFieldDispatchFnStart = 0x0173A0F4;
    constexpr uintptr_t kFieldDispatchFnWindow = 0x40;

    bool isFieldDispatchCaller =
        callerGhidraAddr >= kFieldDispatchFnStart &&
        callerGhidraAddr < kFieldDispatchFnStart + kFieldDispatchFnWindow;

    if (isFieldDispatchCaller && g_LastBuilder)
    {
        if (!g_OwnTypeNodeWrapper)
        {
            void* realNode = RealGetTypeNode();
            constexpr size_t kWrapperSlots = 96;
            void** wrapper = new void*[kWrapperSlots];
            std::memset(wrapper, 0, kWrapperSlots * sizeof(void*));
            wrapper[0x10 / 8] = realNode;
            wrapper[0x40 / 8] = g_LastBuilder;
            g_OwnTypeNodeWrapper = wrapper;
        }

        return g_OwnTypeNodeWrapper;
    }

    return RealGetTypeNode();
}

static void* PassDamageArmor_Allocator()
{
    void* obj = ::operator new(Layout::ExtendedObjectSize);
    RealArmorCtor(obj);
    std::memset((uint8_t*)obj + Layout::PassDamageFlags, 0, Layout::ExtraFieldSize);

    if (!g_ClonedPropSheetVtable)
    {
        void** realVtable = *reinterpret_cast<void***>(obj);
        g_ClonedPropSheetVtable = new void*[PROP_SHEET_VTABLE_SLOT_COUNT];
        std::memcpy(g_ClonedPropSheetVtable, realVtable, PROP_SHEET_VTABLE_SLOT_COUNT * sizeof(void*));

        RealGetTypeNode = reinterpret_cast<GetOwnTypeNode_t>(realVtable[0]);
        g_ClonedPropSheetVtable[0] = reinterpret_cast<void*>(Hooked_GetOwnTypeNode);
    }
    *reinterpret_cast<void***>(obj) = g_ClonedPropSheetVtable;

    return obj;
}

static void PassDamageArmor_FieldRegistrar(void* ctx, void* builder)
{
    g_LastBuilder = builder;

    using PrepareBuilder_t = void (*)(void*, void*);
    auto prepareBuilder =
        reinterpret_cast<PrepareBuilder_t>(Memory::RVA(Offsets::PrepareBuilderAgainstRegistry));

    if (g_TypeRegistry)
    {
        prepareBuilder(g_TypeRegistry, builder);

        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(g_TypeRegistry) + 0x40) = builder;
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(builder) + 0x88) = g_TypeRegistry;
    }
    else
    {
        LOGE("PassDamageArmor_FieldRegistrar: g_TypeRegistry is null");
    }

    auto vtbl = *reinterpret_cast<void***>(ctx);
    auto findTypeByName = reinterpret_cast<FindTypeByName_t>(vtbl[0x58 / 8]);
    auto registerField   = reinterpret_cast<RegisterField_t>(vtbl[0x68 / 8]);
    auto setBaseType      = reinterpret_cast<SetBaseType_t>(vtbl[0x60 / 8]);

    SSOString baseName("ArmorPropertySheet");
    void* baseType = findTypeByName(ctx, baseName.ptr());
    if (baseType)
    {
        auto baseVtbl = *reinterpret_cast<void***>(baseType);
        auto getTypeHandle = reinterpret_cast<void* (*)(void*)>(baseVtbl[0x68 / 8]);
        void* baseHandle = getTypeHandle(baseType);

        if (setBaseType)
        {
            setBaseType(ctx, builder, baseHandle, 0);
        }
        else
        {
            LOGE("PassDamageArmor_FieldRegistrar: setBaseType fn ptr is null");
        }
    }
    else
    {
        LOGE("PassDamageArmor_FieldRegistrar: FindTypeByName(\"ArmorPropertySheet\") returned null");
    }

    auto getProjectileRefVectorType =
        reinterpret_cast<GetVectorType_t>(Memory::RVA(Offsets::GetProjectileRefVectorType));

    SSOString projectileVectorTypeName("std::vector<RtWeakPtr<ProjectilePropertySheet>>");
    void* passDamageType = getProjectileRefVectorType(ctx, projectileVectorTypeName.ptr());

    if (passDamageType)
    {
        SSOString fieldName("PassDamageFlags");
        registerField(ctx, builder, fieldName.ptr(), Layout::PassDamageFlags, passDamageType);
    }
    else
    {
        LOGE("PassDamageArmor_FieldRegistrar: GetProjectileRefVectorType returned null");
    }
}

bool RegisterPassDamageArmorClass()
{
    RealArmorCtor = reinterpret_cast<RealCtor_t>(Memory::RVA(Offsets::ArmorPropertySheet_Ctor));

    void* localRegistry = nullptr;
    RECOVER_FROM_ABORT(
        localRegistry = ::operator new(0x48);
        auto ctorHelper = reinterpret_cast<CtorHelper_t>(Memory::RVA(Offsets::CtorHelper));
        ctorHelper(localRegistry);
    );

    if (!localRegistry)
    {
        LOGE("RegisterPassDamageArmorClass: failed to build local type registry");
        return false;
    }
    g_TypeRegistry = localRegistry;

    auto getArmorBaseType = reinterpret_cast<GetArmorBaseType_t>(
        Memory::RVA(Offsets::ArmorPropertySheetBaseTypeSingleton));
    void* armorBaseType = nullptr;
    RECOVER_FROM_ABORT(
        armorBaseType = getArmorBaseType();
    );

    if (!armorBaseType)
    {
        LOGE("RegisterPassDamageArmorClass: ArmorPropertySheet base type singleton returned null");
        return false;
    }

    auto typeVtbl = *reinterpret_cast<void***>(localRegistry);
    auto registerType = reinterpret_cast<RegisterType_t>(typeVtbl[0x40 / 8]);

    registerType(localRegistry, "PassDamageArmorPropertySheet", armorBaseType, (void*)PassDamageArmor_Allocator);

    auto getPropertyRegistry  = reinterpret_cast<GetSingleton_t>(Memory::RVA(Offsets::GetPropertyRegistry));
    auto findPropertyRegistry = reinterpret_cast<FindInRegistry_t>(Memory::RVA(Offsets::FindPropertyRegistry));

    void* probe = getPropertyRegistry();
    if (!probe) { LOGE("RegisterPassDamageArmorClass: GetPropertyRegistry() returned null"); return false; }
    void* classRegistry = findPropertyRegistry(probe);
    if (!classRegistry) { LOGE("RegisterPassDamageArmorClass: FindPropertyRegistry() returned null"); return false; }

    auto classVtbl = *reinterpret_cast<void***>(classRegistry);
    auto registerClass = reinterpret_cast<RegisterClass_t>(classVtbl[0x28 / 8]);

    registerClass(classRegistry, "PassDamageArmorPropertySheet", (void*)PassDamageArmor_FieldRegistrar,
                  Layout::ExtendedObjectSize, 0);

    void* factoryRegistrant = ::operator new(0x50);
    std::memset(factoryRegistrant, 0, 0x50);
    RECOVER_FROM_ABORT(
        auto ctorHelper2 = reinterpret_cast<CtorHelper_t>(Memory::RVA(Offsets::CtorHelper));
        ctorHelper2(factoryRegistrant);
    );

    using RegisterObjClassFactory_t = void (*)(void*, const char*, void*, void*);
    auto registerObjClassFactory =
        reinterpret_cast<RegisterObjClassFactory_t>(Memory::RVA(Offsets::RegisterObjClassFactory));

    RECOVER_FROM_ABORT(
        registerObjClassFactory(factoryRegistrant, "PassDamageArmorPropertySheet",
                                 armorBaseType, (void*)PassDamageArmor_Allocator);
    );

    LOGI("RegisterPassDamageArmorClass: PassDamageArmorPropertySheet registered successfully");
    return true;
}
