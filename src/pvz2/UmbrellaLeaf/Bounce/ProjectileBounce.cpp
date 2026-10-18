#include "ProjectileBounce.h"
#include "../Config.h"
#include "../Offsets/Layouts.h"
#include "../Offsets/PlantFramework.h"
#include "../Offsets/PlantPropertySheet.h"
#include "helpers/logger.h"
#include <cstdlib>
#include <cstring>

namespace UmbrellaLeaf::Bounce
{
    namespace
    {
        uint64_t g_handled[16] = {};
        uint32_t g_handledHead = 0;
        bool IsHandled(uint64_t r) { for (uint64_t v : g_handled) if (v == r) return true; return false; }
        void MarkHandled(uint64_t r) { g_handled[g_handledHead++ % 16] = r; }

        struct Pending
        {
            uint64_t selfRef;
            void*    owner;
            uint8_t* plant;
            int      dist;
            int      height;
            float    time;
            float    damageMult;
            uint32_t born;
            char     audio[64];
        };
        Pending  g_pending[16] = {};
        uint32_t g_clock = 0;
        constexpr uint32_t kMaxAge = 600;

        template <class T> T Read(const uint8_t* base, uint32_t off)
        {
            return *reinterpret_cast<const T*>(base + off);
        }
        bool ReflectDamage(const uint8_t* proj, uint8_t* plant, float multiplier)
        {
            if (!plant || multiplier <= 0.0f) return false;

            const uint8_t* props = static_cast<const uint8_t*>(
                Engine::ResolveRef(Read<uint64_t>(proj, ProjectileLayout::TypeRef)));
            if (!props) return false;

            const float damage   = Read<float>(props, ProjectilePropsLayout::BaseDamage) * multiplier;
            const float hpBefore = Engine::PlantHealth(plant);
            Engine::DamagePlant(plant, damage);
            const float hpAfter  = Engine::PlantHealth(plant);
            UL_DEBUG("[Bounce] reflect: %.1f x %.2f -> plant hp %.1f -> %.1f", damage / multiplier, multiplier, hpBefore, hpAfter);
            return hpBefore > 0.0f && hpAfter <= 0.0f;
        }
    }

    bool ReadBounceList(const uint8_t* props, const uint64_t*& begin, const uint64_t*& end)
    {
        begin = Read<const uint64_t*>(props, UmbrellaLayout::BounceableProjectiles);
        end   = Read<const uint64_t*>(props, UmbrellaLayout::BounceableProjectiles + 8);
        if (begin == end) return true;
        if (begin > end) return false;
        if (reinterpret_cast<uintptr_t>(begin) & 7) return false;
        return (end - begin) <= 64;
    }

    void Tick()
    {
        ++g_clock;
        for (Pending& p : g_pending)
            if (p.selfRef && g_clock - p.born > kMaxAge) p = Pending{};
    }

    bool TryQueue(uint8_t* proj, const Context& ctx)
    {
        const int projRow  = Read<int>(proj, ProjectileLayout::Row);
        const int plantRow = Read<int>(ctx.plant, PlantLayout::Row);
        if (std::abs(projRow - plantRow) > 1) return false;

        const Engine::Rect pr = Engine::GetProjectileRect(proj);
        const Engine::Rect& t = ctx.trigger;
        if (pr.x + pr.w <= t.x || pr.x >= t.x + t.w || pr.y + pr.h <= t.y || pr.y >= t.y + t.h)
            return false;

        const uint64_t typeRef = Read<uint64_t>(proj, ProjectileLayout::TypeRef);
        bool match = false;
        for (const uint64_t* p = ctx.listBegin; p != ctx.listEnd; ++p)
            if (*p == typeRef) { match = true; break; }
        if (!match) return false;

        const uint64_t selfRef = Read<uint64_t>(proj, ProjectileLayout::SelfRef);
        if (selfRef == 0 || IsHandled(selfRef)) return false;

        // 5) queue it
        Pending* slot = nullptr;
        for (Pending& p : g_pending) if (p.selfRef == 0) { slot = &p; break; }
        if (!slot) return false;

        slot->selfRef    = selfRef;
        slot->owner      = ctx.owner;
        slot->plant      = const_cast<uint8_t*>(ctx.plant);
        slot->dist       = Engine::ScaleToBoard(Read<int>(ctx.props, UmbrellaLayout::ProjectileBounceDistance));
        slot->height     = Engine::ScaleToBoard(Read<int>(ctx.props, UmbrellaLayout::ProjectileBounceHeight));
        slot->time       = Read<float>(ctx.props, UmbrellaLayout::ProjectileBounceTime);
        slot->damageMult = Read<float>(ctx.props, UmbrellaLayout::DamageMultiplier);
        slot->born       = g_clock;
        const char* ev = Engine::ReadStdString(ctx.props + UmbrellaLayout::AudioEventName);
        std::strncpy(slot->audio, ev ? ev : "", sizeof(slot->audio) - 1);
        slot->audio[sizeof(slot->audio) - 1] = 0;

        MarkHandled(selfRef);
        UL_DEBUG("[Bounce] queued proj=%p row=%d rect=(%d,%d,%d,%d)", proj, projRow, pr.x, pr.y, pr.w, pr.h);
        return true;
    }

    int FlushPending(void* owner)
    {
        int bounced = 0;
        bool died = false;
        for (Pending& p : g_pending)
        {
            if (p.selfRef == 0 || p.owner != owner) continue;
            if (died)
            {
                UL_DEBUG("[Bounce] umbrella died: projectile %llx is not bounced", (unsigned long long)p.selfRef);
                p = Pending{};
                continue;
            }

            uint8_t* proj = static_cast<uint8_t*>(Engine::ResolveRef(p.selfRef));
            if (proj)
            {
                const float x = Read<float>(proj, ProjectileLayout::PosX);
                const float y = Read<float>(proj, ProjectileLayout::PosY);
                const float z = Read<float>(proj, ProjectileLayout::PosZ);
                Engine::LaunchProjectile(proj, x + static_cast<float>(p.dist), y, z,
                                         static_cast<float>(p.height), p.time);
                Engine::PlaySound(p.audio);
                ++bounced;
                if (ReflectDamage(proj, p.plant, p.damageMult)) died = true;
                UL_DEBUG("[Bounce] bounced proj=%p from=(%.1f,%.1f,%.1f) dist=%d h=%d t=%.2f",
                         proj, x, y, z, p.dist, p.height, p.time);
            }
            else
            {
                UL_DEBUG("[Bounce] queued projectile %llx is gone, skipped", (unsigned long long)p.selfRef);
            }
            p = Pending{};
        }
        return bounced;
    }
}
