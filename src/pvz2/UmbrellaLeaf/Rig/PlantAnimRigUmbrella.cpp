#include "../Internal.h"
#include "../Anim/UmbrellaAnim.h"
#include "../Common/Reflection.h"
#include "../Offsets/Engine.h"
#include "../Offsets/Layouts.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include "memory/memory.h"
#include <cstring>
#include <new>

namespace UmbrellaLeaf
{
    namespace
    {
        using GetSingleton_t = void* (*)();
        using Allocator_t    = void* (*)();
        using Slot_t         = void  (*)(void*);

        Reflect::TypeNodeSlot g_slot;
        void** g_RigVtable    = nullptr;
        Slot_t RealOnFinished = nullptr;

        void* Hooked_GetTypeNode()
        {
            return Reflect::HookedTypeNode(g_slot, __builtin_return_address(0));
        }

        void Hooked_OnFinished(void* rig)
        {
            if (Anim::HandleRigFinished(rig))
                return;
            RealOnFinished(rig);
        }

        void** CloneVtable()
        {
            if (g_RigVtable) return g_RigVtable;

            auto real = reinterpret_cast<void**>(Memory::RVA(Offsets::Vtable_PlantAnimRig));
            g_RigVtable = new void*[RigLayout::VtableSlotCount];
            std::memcpy(g_RigVtable, real, RigLayout::VtableSlotCount * sizeof(void*));

            g_slot.realGetter = reinterpret_cast<void* (*)()>(g_RigVtable[0]);
            g_RigVtable[0] = reinterpret_cast<void*>(Hooked_GetTypeNode);

            const uint32_t idx = RigLayout::OnFinishedSlot / 8;
            RealOnFinished = reinterpret_cast<Slot_t>(g_RigVtable[idx]);
            g_RigVtable[idx] = reinterpret_cast<void*>(Hooked_OnFinished);

            LOGI("PlantAnimRig_Umbrella: vtable cloned %p", (void*)g_RigVtable);
            return g_RigVtable;
        }

        void* RigAllocator()
        {
            void* obj = reinterpret_cast<Allocator_t>(Memory::RVA(Offsets::Fn_PlantAnimRigAlloc))();
            if (obj)
            {
                Anim::ForgetRig(obj);
                *reinterpret_cast<void***>(obj) = CloneVtable();
                LOGI("PlantAnimRig_Umbrella: allocated rig %p", obj);
            }
            return obj;
        }

        void RigFieldRegistrar(void* ctx, void* builder)
        {
            Reflect::BindBuilder(g_slot, builder);
            Reflect::SetBaseByName(ctx, builder, "PlantAnimRig");
        }
    }

    void** GetUmbrellaRigVtable() { return g_RigVtable; }

    bool RegisterRig()
    {
        void* baseNode = nullptr;
        RECOVER_FROM_ABORT(
            baseNode = reinterpret_cast<GetSingleton_t>(Memory::RVA(Offsets::Fn_PlantAnimRigTypeGetter))();
        );
        if (!baseNode) { LOGE("RegisterRig: PlantAnimRig type node is null"); return false; }

        g_slot.typeNode = Reflect::NewTypeNode();
        if (!g_slot.typeNode) { LOGE("RegisterRig: failed to build type node"); return false; }

        if (!Reflect::RegisterClass(g_slot.typeNode, "PlantAnimRig_Umbrella", baseNode, (void*)RigAllocator,
                                    (void*)RigFieldRegistrar, RigLayout::ObjectSize))
            return false;
        Reflect::RegisterObjClassFactory("PlantAnimRig_Umbrella", baseNode, (void*)RigAllocator);

        LOGI("RegisterRig: PlantAnimRig_Umbrella registered");
        return true;
    }
}
