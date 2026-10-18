#include "crashlog.h"
#include "abortrecovery.h"
#include "helpers/logger.h"
#include <csignal>
#include <ucontext.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <cstdint>

namespace CrashLog
{
    static struct sigaction g_prev[32];
    static bool g_installed = false;
    static thread_local bool t_inHandler = false;

    static void DescribePc(const char* label, uintptr_t pc)
    {
        Dl_info info{};
        if (dladdr(reinterpret_cast<void*>(pc), &info) && info.dli_fname)
        {
            const char* name = std::strrchr(info.dli_fname, '/');
            name = name ? name + 1 : info.dli_fname;
            uintptr_t rva = pc - reinterpret_cast<uintptr_t>(info.dli_fbase);
            if (std::strstr(info.dli_fname, "libPVZ2.so"))
                LOGE("  %s = %s rva=0x%lx ghidra=0x%lx", label, name,
                     (unsigned long)rva, (unsigned long)(rva + 0x100000));
            else
                LOGE("  %s = %s +0x%lx", label, name, (unsigned long)rva);
        }
        else
        {
            LOGE("  %s = %p (unknown module)", label, reinterpret_cast<void*>(pc));
        }
    }

    static void Handler(int sig, siginfo_t* info, void* ctx)
    {
        if (!t_inHandler)
        {
            t_inHandler = true;
            auto* uc = static_cast<ucontext_t*>(ctx);
            auto& mc = uc->uc_mcontext;
            LOGE("CrashLog: signal %d code=%d fault_addr=%p", sig, info->si_code, info->si_addr);
            DescribePc("pc", mc.pc);
            DescribePc("lr", mc.regs[30]);
            for (int i = 0; i < 31; i += 4)
            {
                char buf[160];
                int n = 0;
                for (int j = i; j < i + 4 && j < 31; ++j)
                    n += std::snprintf(buf + n, sizeof(buf) - n, "x%d=%lx ", j, (unsigned long)mc.regs[j]);
                LOGE("  %s", buf);
            }
            const uint32_t* code = reinterpret_cast<const uint32_t*>(mc.pc);
            LOGE("  code pc-16..pc+8: %08x %08x %08x %08x [%08x] %08x %08x",
                code[-4], code[-3], code[-2], code[-1], code[0], code[1], code[2]);
            LOGE("  sp=%p x0=%p x1=%p x2=%p x3=%p", (void*)mc.sp, (void*)mc.regs[0],
                 (void*)mc.regs[1], (void*)mc.regs[2], (void*)mc.regs[3]);
            AbortRecovery::LogBacktrace("fatal signal");
            t_inHandler = false;
        }

        const struct sigaction& prev = g_prev[sig];
        if ((prev.sa_flags & SA_SIGINFO) && prev.sa_sigaction)
            prev.sa_sigaction(sig, info, ctx);
        else if (prev.sa_handler && prev.sa_handler != SIG_DFL && prev.sa_handler != SIG_IGN)
            prev.sa_handler(sig);
        else
            signal(sig, SIG_DFL);   // returning re-faults with the default action
    }

    bool Install()
    {
        if (g_installed) return true;

        struct sigaction sa{};
        sa.sa_sigaction = Handler;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
        sigemptyset(&sa.sa_mask);

        for (int sig : {SIGSEGV, SIGBUS, SIGILL})
            sigaction(sig, &sa, &g_prev[sig]);

        g_installed = true;
        LOGI("CrashLog::Install: handlers installed");
        return true;
    }
}