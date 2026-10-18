#include "UmbrellaLeaf.h"
#include "Internal.h"
#include "helpers/logger.h"

namespace UmbrellaLeaf
{
    bool Install()
    {
        bool ok = true;

        ok &= RegisterFramework();
        ok &= RegisterRig();
        ok &= RegisterPropertySheet();
        if (!ok)
            LOGE("UmbrellaLeaf::Install: one or more steps failed - see above");

        return ok;
    }
}