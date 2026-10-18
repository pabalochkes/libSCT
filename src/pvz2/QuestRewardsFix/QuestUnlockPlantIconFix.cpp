#include "QuestUnlockPlantIconFix.h"
#include "memory/memory.h"
#include <dobby.h>
#include <cstdint>
#include <cstring>

namespace QuestUnlockPlantIconFix
{
    constexpr uintptr_t kRva_LdrW1    = 0x00cf3bcc;
    constexpr uintptr_t kRva_CmpW1_18 = 0x00cf3bd0;

    constexpr uint32_t kWord_LdrW1    = 0xb9400101;
    constexpr uint32_t kWord_CmpW1_18 = 0x7100603f;

    static bool WordMatches(uintptr_t addr, uint32_t expected)
    {
        uint32_t word;
        std::memcpy(&word, reinterpret_cast<const void*>(addr), sizeof(word));
        return word == expected;
    }

    static void InstrumentCallback(RegisterContext* reg_ctx, const HookEntryInfo* info)
    {
        (void)info;

        uint64_t x1 = reg_ctx->general.regs.x1;
        if (static_cast<uint32_t>(x1 & 0xffffffffu) == 3)
        {
            reg_ctx->general.regs.x1 = (x1 & 0xffffffff00000000ull) | 0x18u;
        }
    }

    bool Install()
    {
        uintptr_t base = Memory::GetBase();
        if (base == 0)
        {
            return false;
        }

        uintptr_t ldrAddr = base + kRva_LdrW1;
        uintptr_t cmpAddr = base + kRva_CmpW1_18;

        if (!WordMatches(ldrAddr, kWord_LdrW1) || !WordMatches(cmpAddr, kWord_CmpW1_18))
        {
            return false;
        }
        return DobbyInstrument(reinterpret_cast<void*>(cmpAddr), InstrumentCallback) == 0;
    }
}