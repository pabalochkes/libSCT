#include "EngineApi.h"
#include "../Offsets/Engine.h"
#include "../Offsets/Layouts.h"
#include "../Offsets/PlantFramework.h"
#include "memory/memory.h"
#include <cstring>

namespace UmbrellaLeaf::Engine
{
    namespace
    {
        struct RetBuf32 { uint64_t v[4]; };
        struct IterBuf  { uint64_t v[16]; };
        struct RectBuf  { int32_t  v[8]; };

        using GetMgr_t       = void*    (*)();
        using WeakCtor_t     = void     (*)(void*);
        using WeakAssign_t   = void     (*)(void* dst, const void* src);
        using WeakDtor_t     = void     (*)(void*);
        using WeakResolve_t  = RetBuf32 (*)(void* mgr, void* weak);
        using WeakUInt_t     = uint32_t (*)(void* weak);
        using MgrLookup_t    = void*    (*)(void* mgr, uint32_t handle);
        using EntryDeref_t   = void*    (*)(void* entry, void* weak);
        using IterBegin_t    = IterBuf  (*)(void* container, uint32_t typeId);
        using IterHasNext_t  = uint32_t (*)(void* it);
        using IterCurrent_t  = RetBuf32 (*)(void* it);
        using IterVoid_t     = void     (*)(void* it);
        using ProjRect_t     = RectBuf  (*)(void* proj);
        using ProjSetState_t = void     (*)(void* proj, int value);
        using ProjLaunch_t   = void     (*)(float height, float time, void* proj, float* target);
        using ScaleInt_t     = int      (*)(void* scaleObj, int value);
        using PlayAudio_t    = void     (*)(void* audioMgr, const char* eventName, void* extra);
        using ReceiveDamage_t = void    (*)(void* plant, void* damageInfo);

        constexpr uint32_t kEntityType_Projectile = 0x45;

        template <class T> T Fn(uintptr_t rva)     { return reinterpret_cast<T>(Memory::RVA(rva)); }
        template <class T> T Global(uintptr_t rva) { return *reinterpret_cast<T*>(Memory::RVA(rva)); }
    }

    void* ResolveRef(uint64_t ref)
    {
        uint64_t tmp = 0;
        Fn<WeakCtor_t>(Offsets::Fn_WeakCtor)(&tmp);
        Fn<WeakAssign_t>(Offsets::Fn_WeakAssign)(&tmp, &ref);
        void* mgr = Fn<GetMgr_t>(Offsets::Fn_GetObjMgr)();

        RetBuf32 out = Fn<WeakResolve_t>(Offsets::Fn_WeakResolve)(mgr, &tmp);
        void* obj = nullptr;
        if ((Fn<WeakUInt_t>(Offsets::Fn_WeakIsInvalid)(&out) & 1) == 0)
        {
            uint32_t handle = Fn<WeakUInt_t>(Offsets::Fn_WeakHandle)(&out);
            void* entry = Fn<MgrLookup_t>(Offsets::Fn_MgrLookup)(mgr, handle);
            if (entry)
                obj = Fn<EntryDeref_t>(Offsets::Fn_EntryDeref)(entry, &out);
        }
        Fn<WeakDtor_t>(Offsets::Fn_WeakDtor)(&out);
        Fn<WeakDtor_t>(Offsets::Fn_WeakDtor)(&tmp);
        return obj;
    }

    const char* ReadStdString(const uint8_t* s)
    {
        if (s[0] & 1) return *reinterpret_cast<const char* const*>(s + 16);
        return reinterpret_cast<const char*>(s + 1);
    }

    int ForEachProjectile(EntityVisitor visit, void* user)
    {
        void* container = Global<void*>(Offsets::Global_EntityContainer);
        if (!container) return -1;

        int visited = 0;
        IterBuf it = Fn<IterBegin_t>(Offsets::Fn_IterBegin)(container, kEntityType_Projectile);
        while (Fn<IterHasNext_t>(Offsets::Fn_IterHasNext)(&it) & 1)
        {
            RetBuf32 cur = Fn<IterCurrent_t>(Offsets::Fn_IterCurrent)(&it);
            void* proj = ResolveRef(cur.v[0]);
            Fn<WeakDtor_t>(Offsets::Fn_WeakDtor)(&cur);
            if (proj) { ++visited; visit(proj, user); }
            Fn<IterVoid_t>(Offsets::Fn_IterNext)(&it);
        }
        Fn<IterVoid_t>(Offsets::Fn_IterDtor)(&it);
        return visited;
    }

    Rect GetProjectileRect(void* proj)
    {
        RectBuf r = Fn<ProjRect_t>(Offsets::Fn_ProjectileRect)(proj);
        return Rect{ r.v[0], r.v[1], r.v[2], r.v[3] };
    }

    void LaunchProjectile(void* proj, float tx, float ty, float tz, float height, float time)
    {
        Fn<ProjSetState_t>(Offsets::Fn_ProjectileSetState)(proj, 0);
        float target[3] = { tx, ty, tz };
        Fn<ProjLaunch_t>(Offsets::Fn_ProjectileLaunch)(height, time, proj, target);
    }

    int ScaleToBoard(int value)
    {
        void* scaleObj = Global<void*>(Offsets::Global_ScaleObj);
        return scaleObj ? Fn<ScaleInt_t>(Offsets::Fn_ScaleInt)(scaleObj, value) : value;
    }

    void PlaySound(const char* eventName)
    {
        void* audio = Global<void*>(Offsets::Global_AudioMgr);
        if (audio && eventName && eventName[0])
            Fn<PlayAudio_t>(Offsets::Fn_PlayAudio)(audio, eventName, nullptr);
    }

    float PlantHealth(const void* plant)
    {
        return *reinterpret_cast<const float*>(static_cast<const uint8_t*>(plant) + PlantLayout::Health);
    }

    void DamagePlant(void* plant, float damage)
    {
        alignas(16) uint8_t info[DamageInfoLayout::Size];
        std::memset(info, 0, sizeof(info));
        *reinterpret_cast<void**>(info + DamageInfoLayout::Source) = plant;
        *reinterpret_cast<float*>(info + DamageInfoLayout::Damage) = damage;

        void** vt = *reinterpret_cast<void***>(plant);
        reinterpret_cast<ReceiveDamage_t>(vt[PlantLayout::ReceiveDamageSlot / 8])(plant, info);
    }
}
