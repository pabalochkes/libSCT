#include "RigApi.h"
#include "EngineApi.h"
#include "../Common/SSOString.h"
#include "../Config.h"
#include "../Internal.h"
#include "../Offsets/Engine.h"
#include "../Offsets/Layouts.h"
#include "../Offsets/PlantFramework.h"
#include "helpers/logger.h"
#include "memory/memory.h"
#include <cstring>

namespace UmbrellaLeaf::Engine
{
    namespace
    {
        struct PlayRequest
        {
            uint64_t weak;                 // +0x00 RtWeakPtr
            uint8_t  text[24];             // +0x08 std::string
            void*    vptr;                 // +0x20 PTR_FUN_02476610
            uint8_t  rest[0x58 - 0x28];    // +0x28 plain data
        };
        static_assert(sizeof(PlayRequest) == 0x58, "PlayRequest layout");

        using PlayLabel_t = int  (*)(void* rig, void* label, int zero, void* request);
        using WeakFn_t    = void (*)(void*);
        using Slot_t      = void (*)(void* rig);
    }

    void* GetPlantRig(const uint8_t* plant)
    {
        return ResolveRef(*reinterpret_cast<const uint64_t*>(plant + PlantLayout::RigRef));
    }

    bool RigIsPlantAnimRig(void* rig)
    {
        if (!rig) return false;
        void* vptr = *reinterpret_cast<void**>(rig);
        return vptr == reinterpret_cast<void*>(Memory::RVA(Offsets::Vtable_PlantAnimRig))
            || (GetUmbrellaRigVtable() && vptr == reinterpret_cast<void*>(GetUmbrellaRigVtable()));
    }

    bool RigIsUmbrellaRig(void* rig)
    {
        return rig && GetUmbrellaRigVtable()
            && *reinterpret_cast<void**>(rig) == reinterpret_cast<void*>(GetUmbrellaRigVtable());
    }

    int GetRigState(void* rig)
    {
        if (!RigIsPlantAnimRig(rig)) return -1;
        return *reinterpret_cast<int*>(static_cast<uint8_t*>(rig) + RigLayout::State);
    }

    bool RigPlayLabel(void* rig, const char* label)
    {
        PlayRequest req;
        std::memset(&req, 0, sizeof(req));
        reinterpret_cast<WeakFn_t>(Memory::RVA(Offsets::Fn_WeakCtor))(&req.weak);
        req.vptr = reinterpret_cast<void*>(Memory::RVA(Offsets::Vtable_PlayArgs));

        SSOString name(label);
        const int idx = reinterpret_cast<PlayLabel_t>(Memory::RVA(Offsets::Fn_RigPlayLabel))(
            rig, name.ptr(), 0, &req);

        reinterpret_cast<WeakFn_t>(Memory::RVA(Offsets::Fn_WeakDtor))(&req.weak);

        if (idx == -1) return false;
        *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(rig) + RigLayout::State) = RigState::Attack;
        return true;
    }

    void RigPlayIdle(void* rig)
    {
        void** vt = *reinterpret_cast<void***>(rig);
        reinterpret_cast<Slot_t>(vt[RigLayout::PlayIdleSlot / 8])(rig);
    }

    void* RigSwapUseActionCallback(void* rig, void* newFn)
    {
        void** slot = reinterpret_cast<void**>(static_cast<uint8_t*>(rig) + RigLayout::UseActionDelegate + 8);
        void* cur = *slot;
        if (cur == newFn) return nullptr;
        const uintptr_t base = Memory::GetBase();
        const uintptr_t v = reinterpret_cast<uintptr_t>(cur);
        if (v < base || v >= base + Config::kModuleSize)
        {
            LOGE("[Rig] use_action callback slot holds %p, not a libPVZ2 code pointer: not patching", cur);
            return nullptr;
        }
        *slot = newFn;
        return cur;
    }
}
