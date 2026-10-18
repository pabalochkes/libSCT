#pragma once
#include <cstdint>

namespace UmbrellaLeaf::Anim
{
    void Update(void* framework, uint8_t* plant, bool queuedProjectile);
    void OnPlantFoodStart(uint8_t* plant);
    bool HandleRigFinished(void* rig);
    void ForgetRig(void* rig);
}
