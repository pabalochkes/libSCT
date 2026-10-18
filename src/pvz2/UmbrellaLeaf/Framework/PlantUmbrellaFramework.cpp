#include "../Internal.h"
#include "../Anim/UmbrellaAnim.h"
#include "../Bounce/UmbrellaScan.h"
#include "../Common/Reflection.h"
#include "../Engine/EngineApi.h"
#include "../Engine/RigApi.h"
#include "FrameworkData.h"
#include "../Offsets/Layouts.h"
#include "../Offsets/PlantFramework.h"
#include "../Offsets/Registry.h"
#include "../PlantFood/PlantFood.h"
#include "helpers/logger.h"
#include "memory/abortrecovery.h"
#include "memory/crashlog.h"
#include "memory/memory.h"
#include <cstdint>
#include <cstring>
#include <new>

namespace UmbrellaLeaf
{
    namespace
    {
        struct RectResult { int32_t v[8]; };

        using GetSingleton_t   = void* (*)();
        using Tick_t           = void  (*)(void*);
        using GetTriggerRect_t = RectResult (*)(void* self, uint32_t actionIndex);

        Reflect::TypeNodeSlot g_slot;
        void** g_UmbrellaVtable   = nullptr;
        Tick_t RealFrameworkTick  = nullptr;
        Tick_t RealUsePlantFood   = nullptr;
        Tick_t RealPlantFoodEnd   = nullptr;

        void* Hooked_GetTypeNode()
        {
            return Reflect::HookedTypeNode(g_slot, __builtin_return_address(0));
        }

        uint8_t* PlantOf(void* self)
        {
            return *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(self) + FrameworkLayout::OwnerPlant);
        }

        void Hooked_UmbrellaTick(void* self)
        {
            RealFrameworkTick(self);

            uint8_t* plant = PlantOf(self);
            if (!plant) return;

            FrameworkData& data = GetData(self);
            if (!data.started)
            {
                data.started = 1;
                LOGI("PlantUmbrella: first tick framework=%p plant=%p state=%d row=%d col=%d", self, (void*)plant,
                     *reinterpret_cast<int*>(plant + PlantLayout::State),
                     *reinterpret_cast<int*>(plant + PlantLayout::Row), *reinterpret_cast<int*>(plant + PlantLayout::Col));
            }

            void** vt = *reinterpret_cast<void***>(self);
            auto getRect = reinterpret_cast<GetTriggerRect_t>(vt[Offsets::PlantFramework_TriggerRectSlot / 8]);
            const RectResult r = getRect(self, 0);
            const Engine::Rect trigger{ r.v[0], r.v[1], r.v[2], r.v[3] };

            const bool queued = Scan::Run(self, plant, trigger);
            Anim::Update(self, plant, queued);
        }

        bool Umbrella_CanUsePlantFood(void* self)
        {
            uint8_t* plant = PlantOf(self);
            if (!plant || *reinterpret_cast<int*>(plant + PlantLayout::State) != PlantState::Ready)
                return false;

            return Engine::GetRigState(Engine::GetPlantRig(plant)) < RigState::PlantFoodOn;
        }

        void Umbrella_UsePlantFood(void* self)
        {
            RealUsePlantFood(self);
            uint8_t* plant = PlantOf(self);
            if (!plant) return;

            *reinterpret_cast<int*>(plant + PlantLayout::State) = PlantState::PlantFood;
            Anim::OnPlantFoodStart(plant);
            PlantFood::OnStart(plant);
        }

        void Umbrella_PlantFoodEnd(void* self)
        {
            RealPlantFoodEnd(self);
            uint8_t* plant = PlantOf(self);
            if (plant) *reinterpret_cast<int*>(plant + PlantLayout::State) = PlantState::Ready;
        }

        void** GetUmbrellaVtable()
        {
            if (g_UmbrellaVtable) return g_UmbrellaVtable;

            auto realVtable = reinterpret_cast<void**>(Memory::RVA(Offsets::PlantFramework_Vtable));
            g_UmbrellaVtable = new void*[Offsets::PlantFramework_Vtable_SlotCount];
            std::memcpy(g_UmbrellaVtable, realVtable, Offsets::PlantFramework_Vtable_SlotCount * sizeof(void*));

            g_slot.realGetter = reinterpret_cast<void* (*)()>(g_UmbrellaVtable[0]);
            g_UmbrellaVtable[0] = reinterpret_cast<void*>(Hooked_GetTypeNode);

            RealUsePlantFood = reinterpret_cast<Tick_t>(g_UmbrellaVtable[Offsets::PlantFramework_UsePlantFood / 8]);
            RealPlantFoodEnd = reinterpret_cast<Tick_t>(g_UmbrellaVtable[Offsets::PlantFramework_PlantFoodEnd / 8]);
            RealFrameworkTick = reinterpret_cast<Tick_t>(g_UmbrellaVtable[Offsets::PlantFramework_TickSlot / 8]);

            g_UmbrellaVtable[Offsets::PlantFramework_CanUsePlantFood / 8] = reinterpret_cast<void*>(Umbrella_CanUsePlantFood);
            g_UmbrellaVtable[Offsets::PlantFramework_UsePlantFood / 8]    = reinterpret_cast<void*>(Umbrella_UsePlantFood);
            g_UmbrellaVtable[Offsets::PlantFramework_PlantFoodEnd / 8]    = reinterpret_cast<void*>(Umbrella_PlantFoodEnd);
            g_UmbrellaVtable[Offsets::PlantFramework_TickSlot / 8]        = reinterpret_cast<void*>(Hooked_UmbrellaTick);

            LOGI("PlantUmbrella: vtable cloned %p", (void*)g_UmbrellaVtable);
            return g_UmbrellaVtable;
        }

        void* PlantUmbrella_Allocator()
        {
            CrashLog::Install();

            void* obj = ::operator new(Offsets::UmbrellaFramework_ObjectSize);
            std::memset(obj, 0, Offsets::UmbrellaFramework_ObjectSize);
            *reinterpret_cast<void***>(obj) = GetUmbrellaVtable();
            LOGI("PlantUmbrella: allocated framework %p", obj);
            return obj;
        }

        void PlantUmbrella_FieldRegistrar(void* ctx, void* builder)
        {
            Reflect::BindBuilder(g_slot, builder);
            Reflect::SetBaseByName(ctx, builder, "PlantFramework");
        }
    }

    bool IsUmbrellaFramework(const void* framework)
    {
        return framework && g_UmbrellaVtable
            && *reinterpret_cast<void* const* const*>(framework) == g_UmbrellaVtable;
    }

    bool RegisterFramework()
    {
        void* baseNode = nullptr;
        RECOVER_FROM_ABORT(
            baseNode = reinterpret_cast<GetSingleton_t>(Memory::RVA(Offsets::PlantFramework_TypeNodeGetter))();
        );
        if (!baseNode) { LOGE("RegisterFramework: PlantFramework type node is null"); return false; }

        g_slot.typeNode = Reflect::NewTypeNode();
        if (!g_slot.typeNode) { LOGE("RegisterFramework: failed to build type node"); return false; }

        if (!Reflect::RegisterClass(g_slot.typeNode, "PlantUmbrella", baseNode, (void*)PlantUmbrella_Allocator,
                                    (void*)PlantUmbrella_FieldRegistrar, Offsets::UmbrellaFramework_ObjectSize))
            return false;
        Reflect::RegisterObjClassFactory("PlantUmbrella", baseNode, (void*)PlantUmbrella_Allocator);

        LOGI("RegisterFramework: PlantUmbrella registered");
        return true;
    }
}
