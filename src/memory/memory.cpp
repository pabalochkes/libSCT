#include "memory/memory.h"
#include "helpers/logger.h"

#include <link.h>
#include <cstring>

static uintptr_t g_Base = 0;

static int Callback(dl_phdr_info* info, size_t, void*)
{
    if (!info->dlpi_name)
        return 0;

    if (strstr(info->dlpi_name, "libPVZ2.so"))
    {
        g_Base = info->dlpi_addr;

        LOGI("Found libPVZ2.so");
        LOGI("Base = %p", (void*)g_Base);

        return 1;
    }

    return 0;
}

bool Memory::Initialize()
{
    dl_iterate_phdr(Callback, nullptr);

    if (!g_Base)
    {
        LOGE("Failed to locate libPVZ2.so");
        return false;
    }

    return true;
}

uintptr_t Memory::GetBase()
{
    return g_Base;
}