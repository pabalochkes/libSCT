#pragma once
#include <cstdint>

namespace UmbrellaLeaf
{
    namespace Offsets
    {
        constexpr uintptr_t GetPropertyRegistry           = 0x01624024;
        constexpr uintptr_t FindPropertyRegistry          = 0x0162413C;
        constexpr uintptr_t RegisterObjClassFactory       = 0x0163AC20;
        constexpr uintptr_t CtorHelper                    = 0x0163A40C;
        constexpr uintptr_t PrepareBuilderAgainstRegistry = 0x0163B0DC;
        constexpr uintptr_t GetProjectileRefVectorType    = 0x00C15D14;
    }
}