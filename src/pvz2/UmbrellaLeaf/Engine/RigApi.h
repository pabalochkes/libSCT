#pragma once
#include <cstdint>

namespace UmbrellaLeaf::Engine
{
    void* GetPlantRig(const uint8_t* plant);
    bool  RigIsPlantAnimRig(void* rig);
    bool  RigIsUmbrellaRig(void* rig);
    int   GetRigState(void* rig);
    bool  RigPlayLabel(void* rig, const char* label);
    void  RigPlayIdle(void* rig);
    void* RigSwapUseActionCallback(void* rig, void* newFn);
}
