#pragma once
#include <cstdint>

namespace UmbrellaLeaf::Engine
{
    struct Rect { int32_t x, y, w, h; };
    using EntityVisitor = void (*)(void* entity, void* user);

    void*       ResolveRef(uint64_t ref);
    const char* ReadStdString(const uint8_t* s);

    int         ForEachProjectile(EntityVisitor v, void* user);
    Rect        GetProjectileRect(void* proj);
    void        LaunchProjectile(void* proj, float tx, float ty, float tz, float height, float time);
    int         ScaleToBoard(int value);
    void        PlaySound(const char* eventName);

    void        DamagePlant(void* plant, float damage);
    float       PlantHealth(const void* plant);
}
