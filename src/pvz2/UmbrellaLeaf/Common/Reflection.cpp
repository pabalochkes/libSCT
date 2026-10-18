#include "Reflection.h"
#include "SSOString.h"
#include "../Offsets/Registry.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include "memory/memory.h"
#include <cstring>
#include <new>

namespace UmbrellaLeaf::Reflect
{
    namespace
    {
        using CtorHelper_t     = void  (*)(void*);
        using GetSingleton_t   = void* (*)();
        using FindInRegistry_t = void* (*)(void*);
        using RegisterType_t   = void* (*)(void*, const char*, void*, void*);
        using RegisterClass_t  = void  (*)(void*, void*, void*, uint32_t, uint32_t);
        using FindTypeByName_t = void* (*)(void*, void*);
        using SetBaseType_t    = void  (*)(void*, void*, void*, int);
        using PrepareBuilder_t = void  (*)(void*, void*);
        using GetHandle_t      = void* (*)(void*);
        using RegisterObjClassFactory_t = void (*)(void*, const char*, void*, void*);

        constexpr uint32_t kTypeNodeSize = 0x48;
    }

    void* NewTypeNode()
    {
        void* typeNode = nullptr;
        RECOVER_FROM_ABORT(
            typeNode = ::operator new(kTypeNodeSize);
            reinterpret_cast<CtorHelper_t>(Memory::RVA(Offsets::CtorHelper))(typeNode);
        );
        return typeNode;
    }

    bool RegisterClass(void* typeNode, const char* name, void* baseNode, void* allocator, void* fieldRegistrar, uint32_t objectSize)
    {
        auto typeVtbl = *reinterpret_cast<void***>(typeNode);
        reinterpret_cast<RegisterType_t>(typeVtbl[0x40 / 8])(typeNode, name, baseNode, allocator);

        void* probe = reinterpret_cast<GetSingleton_t>(Memory::RVA(Offsets::GetPropertyRegistry))();
        if (!probe) { LOGE("Reflect: GetPropertyRegistry null (%s)", name); return false; }
        void* classRegistry = reinterpret_cast<FindInRegistry_t>(Memory::RVA(Offsets::FindPropertyRegistry))(probe);
        if (!classRegistry) { LOGE("Reflect: FindPropertyRegistry null (%s)", name); return false; }

        SSOString className(name);
        auto classVtbl = *reinterpret_cast<void***>(classRegistry);
        reinterpret_cast<RegisterClass_t>(classVtbl[0x28 / 8])(
            classRegistry, className.ptr(), fieldRegistrar, objectSize, 0);
        return true;
    }

    bool RegisterObjClassFactory(const char* name, void* baseNode, void* allocator)
    {
        constexpr size_t kRegistrantSize = 0x50;
        void* registrant = ::operator new(kRegistrantSize);
        std::memset(registrant, 0, kRegistrantSize);
        RECOVER_FROM_ABORT(
            reinterpret_cast<CtorHelper_t>(Memory::RVA(Offsets::CtorHelper))(registrant);
        );
        RECOVER_FROM_ABORT(
            reinterpret_cast<RegisterObjClassFactory_t>(Memory::RVA(Offsets::RegisterObjClassFactory))(
                registrant, name, baseNode, allocator);
        );
        return true;
    }

    void* HookedTypeNode(TypeNodeSlot& slot, void* callerAddress)
    {
        const uintptr_t callerGhidraAddr = reinterpret_cast<uintptr_t>(callerAddress) - Memory::GetBase() + 0x100000;

        constexpr uintptr_t kFieldDispatchFnStart  = 0x0173A0F4;
        constexpr uintptr_t kFieldDispatchFnWindow = 0x40;
        const bool isFieldDispatchCaller =
            callerGhidraAddr >= kFieldDispatchFnStart &&
            callerGhidraAddr <  kFieldDispatchFnStart + kFieldDispatchFnWindow;

        if (isFieldDispatchCaller && slot.lastBuilder)
        {
            if (!slot.wrapper)
            {
                constexpr size_t kWrapperSlots = 96;
                void** wrapper = new void*[kWrapperSlots];
                std::memset(wrapper, 0, kWrapperSlots * sizeof(void*));
                wrapper[0x10 / 8] = slot.realGetter();
                wrapper[0x40 / 8] = slot.lastBuilder;
                slot.wrapper = wrapper;
            }
            return slot.wrapper;
        }
        return slot.typeNode ? slot.typeNode : slot.realGetter();
    }

    void BindBuilder(TypeNodeSlot& slot, void* builder)
    {
        slot.lastBuilder = builder;
        if (!slot.typeNode) { LOGE("Reflect: BindBuilder without a type node"); return; }
        BindBuilder(slot.typeNode, builder);
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(slot.typeNode) + 0x40) = builder;
    }

    void BindBuilder(void* typeNode, void* builder)
    {
        reinterpret_cast<PrepareBuilder_t>(Memory::RVA(Offsets::PrepareBuilderAgainstRegistry))(typeNode, builder);
        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(builder) + 0x88) = typeNode;
    }

    bool SetBaseByName(void* ctx, void* builder, const char* baseName)
    {
        auto vtbl           = *reinterpret_cast<void***>(ctx);
        auto findTypeByName = reinterpret_cast<FindTypeByName_t>(vtbl[0x58 / 8]);
        auto setBaseType    = reinterpret_cast<SetBaseType_t>(vtbl[0x60 / 8]);

        SSOString name(baseName);
        void* baseType = findTypeByName(ctx, name.ptr());
        if (!baseType)
        {
            LOGE("Reflect: FindTypeByName(\"%s\") returned null", baseName);
            return false;
        }
        auto baseVtbl  = *reinterpret_cast<void***>(baseType);
        auto getHandle = reinterpret_cast<GetHandle_t>(baseVtbl[0x68 / 8]);
        setBaseType(ctx, builder, getHandle(baseType), 0);
        return true;
    }
}
