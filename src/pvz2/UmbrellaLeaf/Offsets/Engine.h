#pragma once
#include <cstdint>

namespace UmbrellaLeaf::Offsets
{
    // weak references
    constexpr uintptr_t Fn_GetObjMgr     = 0x014EAD90;
    constexpr uintptr_t Fn_WeakCtor      = 0x014D90DC;
    constexpr uintptr_t Fn_WeakAssign    = 0x014D920C;
    constexpr uintptr_t Fn_WeakDtor      = 0x014D9154;
    constexpr uintptr_t Fn_WeakResolve   = 0x014F39CC;
    constexpr uintptr_t Fn_WeakIsInvalid = 0x014D926C;
    constexpr uintptr_t Fn_WeakHandle    = 0x014D9178;
    constexpr uintptr_t Fn_MgrLookup     = 0x014F31A4;
    constexpr uintptr_t Fn_EntryDeref    = 0x014EDE00;

    // entity container iteration
    constexpr uintptr_t Fn_IterBegin     = 0x011BEEF0;
    constexpr uintptr_t Fn_IterHasNext   = 0x014EA3CC;
    constexpr uintptr_t Fn_IterCurrent   = 0x014EA364;
    constexpr uintptr_t Fn_IterNext      = 0x014EA3B0;
    constexpr uintptr_t Fn_IterDtor      = 0x014EA1FC;

    // projectiles
    constexpr uintptr_t Fn_ProjectileRect     = 0x00CA20B8;
    constexpr uintptr_t Fn_ProjectileSetState = 0x00C9F150;
    constexpr uintptr_t Fn_ProjectileLaunch   = 0x00CA2D50;
    constexpr uintptr_t Fn_ScaleInt           = 0x006D0DC0;
    constexpr uintptr_t Fn_PlayAudio          = 0x00A38D18;

    // plant anim rig
    constexpr uintptr_t Fn_PlantAnimRigTypeGetter = 0x005C47C0;
    constexpr uintptr_t Fn_PlantAnimRigAlloc      = 0x005C4890;
    constexpr uintptr_t Fn_RigPlayLabel           = 0x009DBDB8;

    // Effect_PopAnim
    constexpr uintptr_t Fn_GetBoard          = 0x00CE4D8C;
    constexpr uintptr_t Fn_EffectTypeNode    = 0x01230EDC;
    constexpr uintptr_t Fn_CreateEffect      = 0x00AA1EFC;
    constexpr uintptr_t Fn_EffectAttach      = 0x00AC4898;
    constexpr uintptr_t Fn_ResourceRefByName = 0x0105CEF0;
    constexpr uintptr_t Fn_EffectSetPopAnim  = 0x0123251C;
    constexpr uintptr_t Fn_EffectInit        = 0x012325F8;
    constexpr uintptr_t Fn_EffectSetFlag20   = 0x00AC5370;
    constexpr uintptr_t Fn_Effect4944        = 0x00AC4944;
    constexpr uintptr_t Fn_EffectPlayLabel   = 0x01232824;

    // plant food
    constexpr uintptr_t Fn_ExplodeAction     = 0x01275AD4;
    constexpr uintptr_t Fn_PrimalHit         = 0x00E1C608;

    // globals
    constexpr uintptr_t Global_EntityContainer = 0x0256A0D8;
    constexpr uintptr_t Global_AudioMgr        = 0x0256A180;
    constexpr uintptr_t Global_ScaleObj        = 0x025A4618;

    // vtables
    constexpr uintptr_t Vtable_PlayArgs     = 0x02376610;
    constexpr uintptr_t Vtable_PlantAnimRig = 0x02377B40;
}
