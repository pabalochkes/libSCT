#include "UmbrellaScan.h"
#include "ProjectileBounce.h"
#include "../Internal.h"
#include "../Offsets/PlantFramework.h"
#include "../Offsets/PlantPropertySheet.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"

namespace UmbrellaLeaf::Scan
{
    namespace
    {
        bool g_broken = false;
        bool g_badPropsLogged = false;

        struct VisitCtx { Bounce::Context ctx; int queued = 0; };

        void Visit(void* proj, void* user)
        {
            auto* v = static_cast<VisitCtx*>(user);
            if (Bounce::TryQueue(static_cast<uint8_t*>(proj), v->ctx))
                ++v->queued;
        }

        void DumpBadProps(const uint8_t* props)
        {
            void** vptr = *reinterpret_cast<void** const*>(props);
            LOGE("Umbrella: BounceableProjectiles looks invalid. props=%p vptr=%p ourVtable=%p isOurs=%d",
                 (const void*)props, (void*)vptr, (void*)GetPropsVtable(), vptr == GetPropsVtable() ? 1 : 0);
            for (uint32_t off = 0x1B8; off < 0x208; off += 0x20)
            {
                const uint64_t* q = reinterpret_cast<const uint64_t*>(props + off);
                LOGE("  +%03x: %016llx %016llx %016llx %016llx", off,
                     (unsigned long long)q[0], (unsigned long long)q[1],
                     (unsigned long long)q[2], (unsigned long long)q[3]);
            }
        }

        bool ScanImpl(void* owner, uint8_t* plant, const Engine::Rect& trigger)
        {
            const uint64_t propsRef = *reinterpret_cast<const uint64_t*>(plant + PlantLayout::PropsRef);
            const uint8_t* props = static_cast<const uint8_t*>(Engine::ResolveRef(propsRef));
            if (!props) return false;

            VisitCtx vc{ Bounce::Context{ owner, plant, props, nullptr, nullptr, trigger } };
            if (!Bounce::ReadBounceList(props, vc.ctx.listBegin, vc.ctx.listEnd))
            {
                if (!g_badPropsLogged) { g_badPropsLogged = true; DumpBadProps(props); }
                return false;
            }

            Engine::ForEachProjectile(&Visit, &vc);
            return vc.queued > 0;
        }
    }

    bool Run(void* owner, uint8_t* plant, const Engine::Rect& trigger)
    {
        if (g_broken) return false;

        Bounce::Tick();
        volatile bool done = false;
        volatile bool queued = false;
        RECOVER_FROM_ABORT(
            queued = ScanImpl(owner, plant, trigger);
            done = true;
        );
        if (!done)
        {
            g_broken = true;
            LOGE("Umbrella scan faulted, disabled for this session");
            return false;
        }
        return queued;
    }
}
