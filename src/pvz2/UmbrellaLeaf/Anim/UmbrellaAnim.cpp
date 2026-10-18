#include "UmbrellaAnim.h"
#include "../Bounce/ProjectileBounce.h"
#include "../Config.h"
#include "../Engine/EffectApi.h"
#include "../Engine/RigApi.h"
#include "../Framework/FrameworkData.h"
#include "../Internal.h"
#include "../Offsets/Layouts.h"
#include "../Offsets/PlantFramework.h"
#include "../PlantFood/PlantFood.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include <cstring>

namespace UmbrellaLeaf::Anim
{
    namespace
    {
        struct Phase { const char* main; const char* leaf; int frames; };
        constexpr int kPhaseCount = 3;
        constexpr Phase kAttack[kPhaseCount] = {
            { "attack_on",  "attack_on_leaf",  3  },
            { "attack",     "attack",          30 },
            { "attack_off", "attack_off_leaf", 30 } };
        constexpr int kAttackLoopPhase = 1;

        const char* PlantFoodLeafLabel(int rigState)
        {
            switch (rigState)
            {
                case RigState::PlantFoodOn:   return "plantfood_on_leaf";
                case RigState::PlantFoodLoop: return "plantfood";
                case RigState::PlantFoodOff:  return "plantfood_off_leaf";
                default:                      return nullptr;
            }
        }
        struct Binding { void* rig; void* framework; uint8_t* plant; };
        Binding g_bindings[16] = {};

        Binding* FindBinding(void* rig)
        {
            for (Binding& b : g_bindings) if (b.rig == rig) return &b;
            return nullptr;
        }

        void Bind(void* rig, void* framework, uint8_t* plant)
        {
            Binding* b = FindBinding(rig);
            if (!b) for (Binding& c : g_bindings) if (!c.rig) { b = &c; break; }
            if (b) *b = Binding{ rig, framework, plant };
        }

        bool g_animBroken = false;

        void PlayLeaf(FrameworkData& d, uint8_t* plant, const char* label)
        {
            void* eff = Engine::ResolveEffect(d.leafRef);
            if (!eff)       // never spawned, or the effect ended on its own: spawn a new one
            {
                d.leafRef = Engine::SpawnPopAnimEffect(plant, Config::kLeafPopAnim,
                                                       Config::kLeafOffset[0], Config::kLeafOffset[1],
                                                       Config::kLeafOffset[2], Config::kLeafLayer);
                eff = Engine::ResolveEffect(d.leafRef);
                UL_DEBUG("[Anim] leaf effect spawned: ref=%llx effect=%p", (unsigned long long)d.leafRef, eff);
            }
            if (!eff) { LOGE("[Anim] leaf effect could not be created"); return; }
            Engine::EffectPlayLabel(eff, label);
            UL_DEBUG("[Anim] plant=%p leaf -> %s", (void*)plant, label);
        }

        void PlayMain(uint8_t* plant, const char* label)
        {
            void* rig = Engine::GetPlantRig(plant);
            if (!rig || !Engine::RigIsPlantAnimRig(rig))
            {
                static bool logged = false;
                if (!logged) { logged = true; LOGE("[Anim] plant=%p: no PlantAnimRig at plant+0x%x", (void*)plant, PlantLayout::RigRef); }
                return;
            }
            if (std::strcmp(label, "idle") == 0)
            {
                Engine::RigPlayIdle(rig);
                UL_DEBUG("[Anim] plant=%p main -> idle", (void*)plant);
                return;
            }
            const bool ok = Engine::RigPlayLabel(rig, label);
            UL_DEBUG("[Anim] plant=%p main -> %s ok=%d", (void*)plant, label, ok ? 1 : 0);
        }

        void StartPhase(void* fw, FrameworkData& d, uint8_t* plant)
        {
            const Phase& p = kAttack[d.phase];
            UL_DEBUG("[Anim] attack phase %u", d.phase + 1);
            if (p.main) PlayMain(plant, p.main);
            if (p.leaf) PlayLeaf(d, plant, p.leaf);
            d.ticksLeft = p.frames * Config::kTicksPerFrame + 4;
            d.age = 0;

            if (d.phase == kAttackLoopPhase)
                Bounce::FlushPending(fw);
        }

        void Finish(FrameworkData& d, uint8_t* plant)
        {
            PlayMain(plant, "idle");
            d.attacking = 0;
            d.phase = 0;
        }

        void Advance(void* fw, FrameworkData& d, uint8_t* plant)
        {
            if (d.phase + 1 < kPhaseCount) { ++d.phase; StartPhase(fw, d, plant); }
            else Finish(d, plant);
        }

        void StopAttack(FrameworkData& d)
        {
            d.attacking = 0;
            d.phase = 0;
        }

        void FollowPlantFoodLeaf(FrameworkData& d, uint8_t* plant)
        {
            const int rigState = Engine::GetRigState(Engine::GetPlantRig(plant));
            if (rigState >= RigState::PlantFoodOn)
            {
                if (rigState == d.pfRig) return;
                d.pfRig = rigState;
                if (const char* label = PlantFoodLeafLabel(rigState)) PlayLeaf(d, plant, label);
            }
            else
                d.pfRig = 0;
        }

        void TryBind(void* fw, FrameworkData& d, uint8_t* plant)
        {
            void* rig = Engine::GetPlantRig(plant);
            if (!rig || !Engine::RigIsUmbrellaRig(rig)) return;

            Bind(rig, fw, plant);
            PlantFood::InstallUseActionHook(rig);
            d.bound = 1;
            UL_DEBUG("[Anim] umbrella rig bound: rig=%p", rig);
        }

        void UpdateImpl(void* fw, uint8_t* plant, bool queued)
        {
            FrameworkData& d = GetData(fw);

            if (d.bound) FollowPlantFoodLeaf(d, plant);

            const bool plantFoodActive = *reinterpret_cast<int*>(plant + PlantLayout::State) == PlantState::PlantFood;
            if (plantFoodActive && d.attacking)
            {
                UL_DEBUG("[Anim] plant food active: dropping the attack sequence");
                StopAttack(d);
            }

            if (!d.bound && (queued || d.attacking || (++d.probe % 60) == 0))
                TryBind(fw, d, plant);
            if (d.attacking) ++d.age;

            if (queued)
            {
                const int rigState = Engine::GetRigState(Engine::GetPlantRig(plant));

                // plant food: never interrupted, reflect right away
                if (plantFoodActive || rigState >= RigState::PlantFoodOn)
                {
                    Bounce::FlushPending(fw);
                    return;
                }
                if (d.attacking && d.phase == 0)
                    return;

                d.attacking = 1;
                d.phase = 0;
                StartPhase(fw, d, plant);
                return;
            }

            if (d.attacking && d.bound)
            {
                const Phase& p = kAttack[d.phase];
                if (d.age > static_cast<uint32_t>(p.frames * Config::kTicksPerFrame * 3 / 2 + 30))
                {
                    UL_DEBUG("[Anim] watchdog: no finished event for attack phase %u, advancing", d.phase + 1);
                    Advance(fw, d, plant);
                }
            }

            if (!d.attacking || d.bound) return;
            if (--d.ticksLeft <= 0) Advance(fw, d, plant);
        }
    }

    void Update(void* framework, uint8_t* plant, bool queuedProjectile)
    {
        if (g_animBroken) return;
        volatile bool done = false;
        RECOVER_FROM_ABORT(
            UpdateImpl(framework, plant, queuedProjectile);
            done = true;
        );
        if (!done)
        {
            g_animBroken = true;
            LOGE("[Anim] animation update faulted, disabled for this session");
        }
    }

    void OnPlantFoodStart(uint8_t* plant)
    {
        void* fw = FrameworkOf(plant);
        if (!IsUmbrellaFramework(fw)) return;
        FrameworkData& d = GetData(fw);
        if (d.attacking)
        {
            UL_DEBUG("[Anim] plant food starts: dropping the attack sequence");
            StopAttack(d);
        }
    }

    bool HandleRigFinished(void* rig)
    {
        Binding* b = FindBinding(rig);
        if (!b) return false;
        FrameworkData& d = GetData(b->framework);
        if (!d.attacking) return false;
        if (*reinterpret_cast<int*>(b->plant + PlantLayout::State) == PlantState::PlantFood)
            return false;

        UL_DEBUG("[Anim] rig finished event: attack phase %u -> advance", d.phase + 1);
        Advance(b->framework, d, b->plant);
        return true;
    }

    void ForgetRig(void* rig)
    {
        if (Binding* b = FindBinding(rig)) *b = Binding{};
    }
}
