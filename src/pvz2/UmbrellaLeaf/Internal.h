#pragma once

namespace UmbrellaLeaf
{
    bool RegisterFramework();
    bool RegisterPropertySheet();
    bool RegisterRig();

    void** GetPropsVtable();
    void** GetUmbrellaRigVtable();
    bool   IsUmbrellaFramework(const void* framework);
}
