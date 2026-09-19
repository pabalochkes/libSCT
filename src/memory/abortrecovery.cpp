#include "abortrecovery.h"
#include "helpers/logger.h"
#include <dobby.h>
#include <dlfcn.h>
#include <cstdlib>
#include <csignal>

namespace AbortRecovery
{
    thread_local sigjmp_buf g_RecoveryPoint;
    thread_local bool       g_RecoveryArmed = false;

    using Abort_t = void (*)();
    static Abort_t OrigAbort = nullptr;

    using Raise_t = int (*)(int);
    static Raise_t OrigRaise = nullptr;

    static void HookAbort()
    {
        if (g_RecoveryArmed)
        {
            LOGE("AbortRecovery: intercepted a fatal abort() - jumping back to recovery point");
            siglongjmp(g_RecoveryPoint, 1);
        }

        LOGE("AbortRecovery: abort() called with no recovery point armed - letting it die for real");
        OrigAbort();
    }

    static int HookRaise(int sig)
    {
        if (sig == SIGABRT && g_RecoveryArmed)
        {
            LOGE("AbortRecovery: intercepted raise(SIGABRT) - jumping back to recovery point");
            siglongjmp(g_RecoveryPoint, 1);
        }

        return OrigRaise(sig);
    }

    bool Install()
    {
        bool ok = true;

        void* abortTarget = dlsym(RTLD_DEFAULT, "abort");
        if (!abortTarget)
        {
            LOGE("AbortRecovery::Install: could not resolve abort() via dlsym");
            ok = false;
        }
        else if (DobbyHook(abortTarget, (void*)HookAbort, (void**)&OrigAbort) != RT_SUCCESS)
        {
            LOGE("AbortRecovery::Install: DobbyHook on abort() failed");
            ok = false;
        }
        else
        {
            LOGI("AbortRecovery::Install: hooked abort() @ %p, orig=%p", abortTarget, (void*)OrigAbort);
        }

        void* raiseTarget = dlsym(RTLD_DEFAULT, "raise");
        if (!raiseTarget)
        {
            LOGE("AbortRecovery::Install: could not resolve raise() via dlsym");
            ok = false;
        }
        else if (DobbyHook(raiseTarget, (void*)HookRaise, (void**)&OrigRaise) != RT_SUCCESS)
        {
            LOGE("AbortRecovery::Install: DobbyHook on raise() failed");
            ok = false;
        }
        else
        {
            LOGI("AbortRecovery::Install: hooked raise() @ %p, orig=%p", raiseTarget, (void*)OrigRaise);
        }

        return ok;
    }
}
