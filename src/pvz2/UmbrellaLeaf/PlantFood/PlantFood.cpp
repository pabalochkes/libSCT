#include "PlantFood.h"
#include "../Config.h"
#include "../Engine/EngineApi.h"
#include "../Engine/RigApi.h"
#include "../Framework/FrameworkData.h"
#include "../Internal.h"
#include "../Offsets/Engine.h"
#include "../Offsets/Layouts.h"
#include "../Offsets/PlantFramework.h"
#include "../Offsets/PlantPropertySheet.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include "memory/memory.h"
#include <cstring>
#include <new>

namespace UmbrellaLeaf::PlantFood
{
    namespace
    {
        constexpr int kExplodeMode = 6;

        void ApplyPrimalHit(uint8_t* plant, const uint8_t* umbrellaProps, void* zombie)
        {
            auto field = [&](uint32_t off) { return *reinterpret_cast<const float*>(umbrellaProps + off); };

            alignas(16) static uint8_t fakeProps[PrimalLayout::PropsSize];
            alignas(16) static uint8_t fakeProjectile[0x100];
            std::memset(fakeProps, 0, sizeof(fakeProps));
            std::memset(fakeProjectile, 0, sizeof(fakeProjectile));

            *reinterpret_cast<float*>(fakeProps + PrimalLayout::StunDuration)      = field(UmbrellaLayout::StunPFDuration);
            *reinterpret_cast<float*>(fakeProps + PrimalLayout::KnockbackChance)   = field(UmbrellaLayout::KnockbackPFChance);
            *reinterpret_cast<float*>(fakeProps + PrimalLayout::KnockbackDistance) = field(UmbrellaLayout::KnockbackPFDistance);
            *reinterpret_cast<float*>(fakeProps + PrimalLayout::KnockbackTime)     = field(UmbrellaLayout::KnockbackPFTime);
            *reinterpret_cast<float*>(fakeProps + PrimalLayout::KnockbackHeight)   = field(UmbrellaLayout::KnockbackPFHeight);
            *reinterpret_cast<void**>(fakeProjectile + PrimalLayout::ProjectileProps)     = fakeProps;
            *reinterpret_cast<uint64_t*>(fakeProjectile + PrimalLayout::ProjectileShooter) =
                *reinterpret_cast<const uint64_t*>(plant + PlantLayout::SelfRef);

            using PrimalHit_t = void (*)(void* projectile, void* zombie);
            reinterpret_cast<PrimalHit_t>(Memory::RVA(Offsets::Fn_PrimalHit))(fakeProjectile, zombie);
        }

        void ApplyPrimalHitToTargets(uint8_t* plant, const uint8_t* umbrellaProps, void* begin, void* end)
        {
            if (!begin || end <= begin) return;

            const uintptr_t base = Memory::GetBase();
            int applied = 0;
            const uintptr_t* it = static_cast<const uintptr_t*>(begin);
            for (int n = 0; it < static_cast<const uintptr_t*>(end) && n < Config::kMaxExplodeTargets; ++it, ++n)
            {
                void* target = reinterpret_cast<void*>(*it);
                if (!target || reinterpret_cast<uintptr_t>(target) <= 0x10000) continue;

                const uintptr_t vptr = *reinterpret_cast<const uintptr_t*>(target);
                if (vptr < base || vptr >= base + Config::kModuleSize) continue;

                ApplyPrimalHit(plant, umbrellaProps, target);
                ++applied;
            }
            UL_DEBUG("[PF] primal hit applied to %d targets", applied);
        }

        void Explode(uint8_t* plant)
        {
            void* props = Engine::ResolveRef(*reinterpret_cast<uint64_t*>(plant + PlantLayout::PropsRef));
            if (!props) { LOGE("[PF] explode: no props"); return; }

            uint8_t* pb = static_cast<uint8_t*>(props);
            uint8_t* actions = *reinterpret_cast<uint8_t**>(pb + ActionLayout::PropsActionsBegin);
            uint8_t* actionsEnd = *reinterpret_cast<uint8_t**>(pb + ActionLayout::PropsActionsEnd);
            const size_t count = (actions && actionsEnd >= actions)
                ? static_cast<size_t>(actionsEnd - actions) / ActionLayout::Stride : 0;
            if (Config::kExplodeActionIndex >= count)
            {
                LOGE("[PF] explode: action index %u >= action count %zu", Config::kExplodeActionIndex, count);
                return;
            }

            void* fw = FrameworkOf(plant);
            if (!IsUmbrellaFramework(fw)) { LOGE("[PF] explode: plant+0x%x is not our framework", PlantLayout::Framework); return; }

            uint8_t* action = actions + Config::kExplodeActionIndex * ActionLayout::Stride;

            // wider area for plant food only (area explodes), restored right after
            float* radius = reinterpret_cast<float*>(action + ActionLayout::ExplodeRadius);
            const float jsonRadius = *radius;

            struct TargetVector { void* begin; void* end; void* cap; };   // >16 bytes: returned through the hidden x8 pointer
            using ExplodeFn = TargetVector (*)(void* plant, void* action, uint32_t index, int mode);
            using ExecuteFn = void (*)(void* framework);
            void** frameworkVtable = *reinterpret_cast<void***>(fw);

            volatile bool done = false;
            RECOVER_FROM_ABORT(
                TargetVector hit = reinterpret_cast<ExplodeFn>(Memory::RVA(Offsets::Fn_ExplodeAction))(
                    plant, action, Config::kExplodeActionIndex, kExplodeMode);
                ApplyPrimalHitToTargets(plant, pb, hit.begin, hit.end);
                if (hit.begin) ::operator delete(hit.begin);
                reinterpret_cast<ExecuteFn>(frameworkVtable[ActionLayout::ExecuteSlot / 8])(fw);
                done = true;
            );
            *radius = jsonRadius;
            UL_DEBUG("[PF] explode: action %u executed ok=%d", Config::kExplodeActionIndex, done ? 1 : 0);
        }

        using UseActionFn = uint64_t (*)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
        UseActionFn g_realUseAction = nullptr;

        uint64_t Hooked_UseAction(void* self, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
        {
            const uint64_t ret = g_realUseAction ? g_realUseAction(self, a1, a2, a3, a4, a5) : 0;

            uint8_t* plant = static_cast<uint8_t*>(self);
            void* fw = FrameworkOf(plant);
            if (!IsUmbrellaFramework(fw)) return ret;

            FrameworkData& d = GetData(fw);
            const bool plantFood = *reinterpret_cast<int*>(plant + PlantLayout::State) == PlantState::PlantFood;
            UL_DEBUG("[PF] use_action fired: plant=%p plantFood=%d done=%u", self, plantFood ? 1 : 0, d.pfDone);
            if (plantFood && !d.pfDone)
            {
                d.pfDone = 1;
                Explode(plant);
            }
            return ret;
        }
    }

    void InstallUseActionHook(void* rig)
    {
        void* previous = Engine::RigSwapUseActionCallback(rig, reinterpret_cast<void*>(Hooked_UseAction));
        if (previous && !g_realUseAction) g_realUseAction = reinterpret_cast<UseActionFn>(previous);
    }

    void OnStart(uint8_t* plant)
    {
        void* fw = FrameworkOf(plant);
        if (IsUmbrellaFramework(fw)) GetData(fw).pfDone = 0;
    }
}
