#include "EffectApi.h"
#include "EngineApi.h"
#include "../Common/SSOString.h"
#include "../Config.h"
#include "../Offsets/Engine.h"
#include "../Offsets/PlantFramework.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include "memory/memory.h"

namespace UmbrellaLeaf::Engine
{
    namespace
    {
        struct RetBuf32 { uint64_t v[4]; };

        using GetBoard_t   = void*    (*)();
        using TypeNode_t   = void*    (*)();
        using Create_t     = void*    (*)(void* board, void* typeNode);
        using Attach_t     = void     (*)(void* effect, void* owner, float* offset, int layer);
        using RefByName_t  = RetBuf32 (*)(void* name);
        using SetPopAnim_t = void     (*)(void* effect, void* resource, void* rigType);
        using Int_t        = void     (*)(void* effect, int value);
        using PlayLabel_t  = void     (*)(void* effect, void* label, int zero);
        using WeakDtor_t   = void     (*)(void*);

        template <class T> T Fn(uintptr_t rva) { return reinterpret_cast<T>(Memory::RVA(rva)); }

        bool OwnerRenderLayer(void* owner, int& out)
        {
            volatile int layer = 0;
            volatile bool ok = false;
            void** vt = *reinterpret_cast<void***>(owner);
            RECOVER_FROM_ABORT(
                layer = reinterpret_cast<int (*)(void*)>(vt[PlantLayout::RenderLayerSlot / 8])(owner);
                ok = true;
            );
            out = layer;
            return ok;
        }

        int ClampLayerOffset(void* owner, int layer)
        {
            int parentLayer = 0;
            if (!OwnerRenderLayer(owner, parentLayer) || parentLayer <= 0)
                return layer;

            const int maxOffset = Config::kMaxRenderLayer - parentLayer;
            if (layer <= maxOffset)
                return layer;

            UL_DEBUG("[Effect] layer offset %d on parent layer %d would pass %d: clamped to %d",
                     layer, parentLayer, Config::kMaxRenderLayer, maxOffset > 0 ? maxOffset : 0);
            return maxOffset > 0 ? maxOffset : 0;
        }
    }

    uint64_t SpawnPopAnimEffect(void* owner, const char* popAnimName, float ox, float oy, float oz, int layer)
    {
        SSOString name(popAnimName);
        RetBuf32 ref = Fn<RefByName_t>(Offsets::Fn_ResourceRefByName)(name.ptr());
        void* resource = ResolveRef(ref.v[0]);
        Fn<WeakDtor_t>(Offsets::Fn_WeakDtor)(&ref);
        if (!resource)
        {
            LOGE("[Effect] popanim '%s' not found", popAnimName);
            return 0;
        }

        // 2) create the effect entity and attach it to the owner
        void* board    = Fn<GetBoard_t>(Offsets::Fn_GetBoard)();
        void* typeNode = Fn<TypeNode_t>(Offsets::Fn_EffectTypeNode)();
        void* effect   = Fn<Create_t>(Offsets::Fn_CreateEffect)(board, typeNode);
        if (!effect) return 0;

        float offset[3] = { ox, oy, oz };
        Fn<Attach_t>(Offsets::Fn_EffectAttach)(effect, owner, offset, ClampLayerOffset(owner, layer));
        Fn<SetPopAnim_t>(Offsets::Fn_EffectSetPopAnim)(effect, resource, nullptr);
        Fn<Int_t>(Offsets::Fn_EffectInit)(effect, 1);
        Fn<Int_t>(Offsets::Fn_EffectSetFlag20)(effect, 0);
        Fn<Int_t>(Offsets::Fn_Effect4944)(effect, 1);

        return *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(effect) + 8);
    }

    void* ResolveEffect(uint64_t ref) { return ref ? ResolveRef(ref) : nullptr; }

    void EffectPlayLabel(void* effect, const char* label)
    {
        SSOString s(label);
        Fn<PlayLabel_t>(Offsets::Fn_EffectPlayLabel)(effect, s.ptr(), 0);
    }
}
