#include "PassDamageArmor.h"
#include "helpers/logger.h"

bool InstallAll()
{
    bool ok = true;

    ok &= RegisterPassDamageArmorClass();
    ok &= InstallApplyDamageHook();
    ok &= InstallNewspaperArmorApplyDamageHook();
    ok &= InstallProjectileCollisionHook();

    if (!ok)
    {
        LOGE("PassDamageArmor::InstallAll: one or more steps failed - see above");
    }

    return ok;
}
