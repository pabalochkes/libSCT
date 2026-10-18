#include "abortrecovery.h"
#include "helpers/logger.h"
#include <dobby.h>
#include <unwind.h>
#include <cstring>
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
    struct BacktraceState { int index = 0; };

    static _Unwind_Reason_Code BacktraceCallback(struct _Unwind_Context* ctx, void* arg)
    {
        auto* st = static_cast<BacktraceState*>(arg);
        uintptr_t pc = _Unwind_GetIP(ctx);
        if (pc == 0 || st->index >= 24)
            return _URC_END_OF_STACK;

        Dl_info info{};
        if (dladdr(reinterpret_cast<void*>(pc), &info) && info.dli_fname)
        {
            const char* name = std::strrchr(info.dli_fname, '/');
            name = name ? name + 1 : info.dli_fname;
            uintptr_t rva = pc - reinterpret_cast<uintptr_t>(info.dli_fbase);

            if (std::strstr(info.dli_fname, "libPVZ2.so"))
                LOGE("  #%02d %s rva=0x%lx ghidra=0x%lx", st->index, name,
                    (unsigned long)rva, (unsigned long)(rva + 0x100000));
            else
                LOGE("  #%02d %s +0x%lx %s", st->index, name,
                    (unsigned long)rva, info.dli_sname ? info.dli_sname : "");
        }
        else
        {
            LOGE("  #%02d pc=%p (unknown module)", st->index, (void*)pc);
        }

        st->index++;
        return _URC_NO_REASON;
    }

    void LogBacktrace(const char* why)
    {
        LOGE("AbortRecovery: backtrace (%s):", why);
        BacktraceState st;
        _Unwind_Backtrace(BacktraceCallback, &st);
    }
    static void HookAbort()
    {
        if (g_RecoveryArmed)
        {
            LOGE("AbortRecovery: intercepted a fatal abort() - jumping back to recovery point");
            siglongjmp(g_RecoveryPoint, 1);
        }

        LOGE("AbortRecovery: abort() called with no recovery point armed - letting it die for real");
        LogBacktrace("abort");
        OrigAbort();
    }

    static int HookRaise(int sig)
    {
        if (sig == SIGABRT && g_RecoveryArmed)
        {
            LOGE("AbortRecovery: intercepted raise(SIGABRT) - jumping back to recovery point");
            siglongjmp(g_RecoveryPoint, 1);
        }

        if (sig == SIGABRT)
            LogBacktrace("raise(SIGABRT)");
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
