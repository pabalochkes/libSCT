#include "LawnZoomFix.h"
#include "../../helpers/logger.h"
#include "../../memory/memory.h"
#include <dobby.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace
{
    constexpr uintptr_t kLawnScaleFuncRva   = 0xAA0C40;
    constexpr uintptr_t kAppSingletonPtrRva = 0x2593760;
    constexpr uintptr_t kUiScaleFuncRva     = 0x6D0DC0;
    constexpr uintptr_t kUiScaleObjPtrRva   = 0x25A4618;

    // ==== Field offsets =====================================================
    constexpr uintptr_t kOffX      = 0x438; // int
    constexpr uintptr_t kOffA      = 0x440; // int
    constexpr uintptr_t kOffW      = 0x448; // int
    constexpr uintptr_t kOffH      = 0x44C; // int
    constexpr uintptr_t kOffBgW    = 0x4C;  // int
    constexpr uintptr_t kOffScale  = 0x460; // float
    constexpr uintptr_t kOffEdge   = 0x464; // float
    constexpr uintptr_t kOffBgX    = 0x46C; // int
    constexpr uintptr_t kOffLawnX  = 0x470; // int
    constexpr uintptr_t kLawnPosFuncRva = 0xAA09E0;

    constexpr uintptr_t kOffScreenW = 0xF4; // int
    constexpr uintptr_t kOffScreenH = 0xF8; // int

    // ==== Tunables ==========================================================
    constexpr float kWideScreenScale   = 1.061f; 
    constexpr bool  kRecomputeEdge     = true;
    constexpr bool  kForceOnAllScreens = true;
    constexpr bool  kAutoScale     = true;
    constexpr int   kTopMarginVirt = 120;
    constexpr float kAspect169       = 2730.0f / 1536.0f;
    constexpr float kAspectPhone     = 3413.0f / 1536.0f;
    constexpr int   kShiftAt169      = 90;
    constexpr int   kShiftAtPhone    = 198;

    constexpr float kBoostPerAspect  = 0.26f;
    constexpr int kMaxLawnX = 1380;
    constexpr float kExtraPerAspect = 170.0f;
    constexpr int   kMaxWideExtra   = 60;

    using LawnScaleFn = void (*)(void*);
    using UiScaleFn   = int (*)(void*, int);
    LawnScaleFn g_origLawnScale = nullptr;
    LawnScaleFn g_origLawnPos   = nullptr;
    int g_bgW = 0;

    template <typename T>
    T& Field(void* obj, uintptr_t off)
    {
        return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + off);
    }

    void* FollowExistingHook(void* entry)
    {
        for (int i = 0; i < 4; ++i)
        {
            const uint32_t* p = reinterpret_cast<const uint32_t*>(entry);
            const uint32_t insn = p[0];
            void* next = nullptr;

            if ((insn & 0xFC000000u) == 0x14000000u)
            {
                const int32_t imm = static_cast<int32_t>((insn & 0x03FFFFFFu) << 6) >> 6;
                next = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(entry) +
                                               (static_cast<intptr_t>(imm) << 2));
            }
            else if (insn == 0x58000051u && p[1] == 0xD61F0220u)
            {
                uint64_t lit = 0;
                std::memcpy(&lit, p + 2, sizeof(lit));
                next = reinterpret_cast<void*>(lit);
            }
            else
            {
                break;
            }

            LOGI("LawnZoomFix: %p already hooked (insn=0x%08x), following to %p", entry, insn, next);
            entry = next;
        }
        return entry;
    }

    bool GetScreen(int& w, int& h)
    {
        const uintptr_t base = static_cast<uintptr_t>(Memory::GetBase());
        const uintptr_t app = *reinterpret_cast<uintptr_t*>(base + kAppSingletonPtrRva);
        if (!app)
            return false;
        w = *reinterpret_cast<int*>(app + kOffScreenW);
        h = *reinterpret_cast<int*>(app + kOffScreenH);
        return w > 0 && h > 0;
    }

    void Hooked_LawnPos(void* self)
    {
        g_origLawnPos(self);
        if (!self)
            return;

        int w = 0, h = 0;
        if (!GetScreen(w, h))
            return;
        const bool wide = (h > 1000) && (static_cast<float>(w) / static_cast<float>(h) > 2.0f);

        int& bgX   = Field<int>(self, kOffBgX);
        int& lawnX = Field<int>(self, kOffLawnX);

        {
            const int bw = Field<int>(self, kOffBgW);
            if (bw > 0 && bw != w)
                g_bgW = bw;
        }

        if (!wide && !kForceOnAllScreens)
            return;

        const float curScale = Field<float>(self, kOffScale);
        const float curEdge  = Field<float>(self, kOffEdge);
        if (curScale > 0.0f && curEdge > 0.0f)
        {
            const float aspect = static_cast<float>(w) / static_cast<float>(h);
            float t = (aspect - kAspect169) / (kAspectPhone - kAspect169);
            t = std::max(0.0f, std::min(1.0f, t));
            const int shift = static_cast<int>(std::lround(
                static_cast<float>(kShiftAt169) + t * static_cast<float>(kShiftAtPhone - kShiftAt169)));
            int target = lawnX + shift;
            if (kMaxLawnX > 0)
            {
                int wideExtra = 0;
                if (aspect > kAspectPhone)
                    wideExtra = std::min(kMaxWideExtra,
                        static_cast<int>(std::lround(kExtraPerAspect * (aspect - kAspectPhone))));
                target = std::min(target, std::max(kMaxLawnX, lawnX + wideExtra));
            }
            lawnX = target;
        }
    }

    void Hooked_LawnScale(void* self)
    {
        g_origLawnScale(self);

        if (!self)
            return;

        const uintptr_t base = static_cast<uintptr_t>(Memory::GetBase());
        const uintptr_t app = *reinterpret_cast<uintptr_t*>(base + kAppSingletonPtrRva);
        if (!app)
            return;

        const int w = *reinterpret_cast<int*>(app + kOffScreenW);
        const int h = *reinterpret_cast<int*>(app + kOffScreenH);
        if (w <= 0 || h <= 0)
            return;

        float& scale = Field<float>(self, kOffScale);
        float& edge  = Field<float>(self, kOffEdge);

        const bool wide = (h > 1000) && (static_cast<float>(w) / static_cast<float>(h) > 2.0f);

        if (!wide && !kForceOnAllScreens)
            return;

        if (kRecomputeEdge)
        {
            void* uiObj = *reinterpret_cast<void**>(base + kUiScaleObjPtrRva);
            auto uiScale = reinterpret_cast<UiScaleFn>(base + kUiScaleFuncRva);
            if (uiObj)
            {
                const int sum = Field<int>(self, kOffW) + Field<int>(self, kOffA);
                edge = static_cast<float>(uiScale(uiObj, sum) + Field<int>(self, kOffX));
            }
        }

        float s = kWideScreenScale;
        if (kAutoScale)
        {
            void* uiObj2 = *reinterpret_cast<void**>(base + kUiScaleObjPtrRva);
            auto uiScale2 = reinterpret_cast<UiScaleFn>(base + kUiScaleFuncRva);
            const int bgW = (g_bgW > 0) ? g_bgW : Field<int>(self, kOffBgW);
            float cover = 0.0f, fitW = 0.0f, fitH = 0.0f;

            if (bgW > 0)
            {
                cover = static_cast<float>(w) / static_cast<float>(bgW) * 1.003f;
                s = std::max(s, cover);
            }
            const float aspectS = static_cast<float>(w) / static_cast<float>(h);
            if (aspectS > kAspectPhone)
                s += kBoostPerAspect * (aspectS - kAspectPhone);

            if (uiObj2)
            {
                const int lawnPx = uiScale2(uiObj2, Field<int>(self, kOffW) + Field<int>(self, kOffA));
                const int topPx = uiScale2(uiObj2, Field<int>(self, kOffH) + kTopMarginVirt);
                if (lawnPx > 0) { fitW = static_cast<float>(w) / static_cast<float>(lawnPx); s = std::min(s, fitW); }
                if (topPx > 0)  { fitH = static_cast<float>(h) / static_cast<float>(topPx);  s = std::min(s, fitH); }
            }
        }
        scale = s;
    }
}

namespace LawnZoomFix
{
    bool Install()
    {
        const uintptr_t base = static_cast<uintptr_t>(Memory::GetBase());
        void* entry  = reinterpret_cast<void*>(base + kLawnScaleFuncRva);
        void* target = FollowExistingHook(entry);

        const int rc = DobbyHook(target,
                                 reinterpret_cast<void*>(Hooked_LawnScale),
                                 reinterpret_cast<void**>(&g_origLawnScale));
        if (rc != 0 || !g_origLawnScale)
        {
            LOGE("LawnZoomFix: DobbyHook failed (rc=%d)", rc);
            return false;
        }
        LOGI("LawnZoomFix: hooked lawn scale at %p (entry %p)", target, entry);

        void* posEntry  = reinterpret_cast<void*>(base + kLawnPosFuncRva);
        void* posTarget = FollowExistingHook(posEntry);
        const int rc2 = DobbyHook(posTarget,
                                  reinterpret_cast<void*>(Hooked_LawnPos),
                                  reinterpret_cast<void**>(&g_origLawnPos));
        if (rc2 != 0 || !g_origLawnPos)
            LOGE("LawnZoomFix: position hook failed (rc=%d), shift disabled", rc2);
        else
            LOGI("LawnZoomFix: hooked lawn position at %p (entry %p)", posTarget, posEntry);
        return true;
    }
}
