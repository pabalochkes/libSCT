#pragma once
#include <cstdint>
#include <cstring>
#include <csignal>
#include <csetjmp>

namespace SafeMem
{
    thread_local sigjmp_buf g_jmpBuf;
    thread_local volatile sig_atomic_t g_inProbe = 0;
    thread_local uint64_t g_lastFaultAddr = 0;

    inline struct sigaction g_prevSegv {};
    inline bool g_installed = false;

    inline void SegvHandler(int sig, siginfo_t* info, void* ucontext)
    {
        if (g_inProbe)
        {
            g_lastFaultAddr = reinterpret_cast<uint64_t>(info ? info->si_addr : nullptr);
            siglongjmp(g_jmpBuf, 1);
            return;
        }
        if (g_prevSegv.sa_flags & SA_SIGINFO && g_prevSegv.sa_sigaction)
        {
            g_prevSegv.sa_sigaction(sig, info, ucontext);
        }
        else if (g_prevSegv.sa_handler && g_prevSegv.sa_handler != SIG_DFL && g_prevSegv.sa_handler != SIG_IGN)
        {
            g_prevSegv.sa_handler(sig);
        }
        else
        {
            signal(SIGSEGV, SIG_DFL);
            raise(SIGSEGV);
        }
    }

    inline void EnsureInstalled()
    {
        if (g_installed) return;
        struct sigaction sa {};
        sa.sa_sigaction = SegvHandler;
        sa.sa_flags = SA_SIGINFO | SA_NODEFER;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGSEGV, &sa, &g_prevSegv);
        g_installed = true;
    }

    inline bool ReadMemory(const void* addr, void* out, size_t len)
    {
        EnsureInstalled();
        g_inProbe = 1;
        if (sigsetjmp(g_jmpBuf, 1) == 0)
        {
            std::memcpy(out, addr, len);
            g_inProbe = 0;
            return true;
        }
        g_inProbe = 0;
        return false;
    }

    inline bool ReadU64(const void* addr, uint64_t& out)
    {
        return ReadMemory(addr, &out, sizeof(out));
    }

    template <typename Func>
    inline bool RunProtected(Func&& fn)
    {
        EnsureInstalled();
        g_inProbe = 1;
        if (sigsetjmp(g_jmpBuf, 1) == 0)
        {
            fn();
            g_inProbe = 0;
            return true;
        }
        g_inProbe = 0;
        return false;
    }
}
