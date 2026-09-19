#pragma once
#include <csetjmp>
#include "helpers/logger.h"

namespace AbortRecovery
{
    bool Install();
    
    extern thread_local sigjmp_buf g_RecoveryPoint;
    extern thread_local bool       g_RecoveryArmed;
}

#define RECOVER_FROM_ABORT(codeBlock)                                        \
    do {                                                                     \
        AbortRecovery::g_RecoveryArmed = true;                               \
        if (sigsetjmp(AbortRecovery::g_RecoveryPoint, 1) == 0) {             \
            codeBlock;                                                       \
        } else {                                                             \
            LOGE("RECOVER_FROM_ABORT: caught a fatal abort at %s:%d, recovered", \
                 __FILE__, __LINE__);                                        \
        }                                                                    \
        AbortRecovery::g_RecoveryArmed = false;                              \
    } while (0)
