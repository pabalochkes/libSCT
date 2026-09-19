#include "../PassDamageArmor.h"
#include "../Internal.h"
#include "../Offsets/Combat.h"
#include "helpers/logger.h"
#include "memory/memory.h"
#include <dobby.h>

using ProjectileCollision_t = void (*)(void*);
static ProjectileCollision_t RealProjectileCollision = nullptr;

thread_local void* g_currentProjectile = nullptr;

static void Hooked_ProjectileCollision(void* param_1)
{
    void* previous = g_currentProjectile;
    g_currentProjectile = param_1;

    RealProjectileCollision(param_1);

    g_currentProjectile = previous;
}

bool InstallProjectileCollisionHook()
{
    void* target = reinterpret_cast<void*>(Memory::RVA(Offsets::ProjectileCollision));

    int result = DobbyHook(
        target,
        reinterpret_cast<void*>(Hooked_ProjectileCollision),
        reinterpret_cast<void**>(&RealProjectileCollision)
    );

    if (result != 0)
    {
        LOGE("InstallProjectileCollisionHook: DobbyHook failed, code=%d", result);
        return false;
    }

    LOGI("InstallProjectileCollisionHook: hooked @ %p", target);
    return true;
}
