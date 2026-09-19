#include "initarray.h"
#include "corrected_ctors.h"
#include "helpers/logger.h"
#include <cstdint>
#include <chrono>

namespace Memory
{
    constexpr size_t SkipIndex = 886;

    bool RunInitArray(void* base)
    {
        auto t0 = std::chrono::steady_clock::now().time_since_epoch().count();
        LOGI("RunInitArray: START t=%lld", (long long)t0);
        LOGI("RunInitArray: invoking %zu corrected constructors (skipping index %zu)",
             CorrectedCtors::Count, SkipIndex);

        auto baseBytes = reinterpret_cast<uint8_t*>(base);
        for (size_t i = 0; i < CorrectedCtors::Count; i++)
        {
            if (i == SkipIndex)
            {
                LOGI("RunInitArray: SKIPPING ctor[%zu] = base+0x%zx (known CheatUI/JNI failure)",
                     i, CorrectedCtors::Offsets[i]);
                continue;
            }

            auto ctor = reinterpret_cast<void (*)()>(baseBytes + CorrectedCtors::Offsets[i]);
            if (i < 10 || i % 100 == 0)
                LOGI("RunInitArray: calling ctor[%zu] = %p (base+0x%zx)",
                     i, (void*)ctor, CorrectedCtors::Offsets[i]);

            ctor();
        }

        auto t1 = std::chrono::steady_clock::now().time_since_epoch().count();
        LOGI("RunInitArray: END t=%lld (elapsed=%lld ns), %zu constructors invoked (1 skipped)",
             (long long)t1, (long long)(t1 - t0), CorrectedCtors::Count - 1);
        return true;
    }
}
