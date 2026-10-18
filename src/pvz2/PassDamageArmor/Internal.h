#pragma once
#include <cstdint>

struct ApplyDamageResult { uint8_t _opaque[0xA0]; };
using ApplyDamage_t = ApplyDamageResult (*)(void*, void*, uint64_t);

extern ApplyDamage_t RealApplyDamage;

ApplyDamageResult Hooked_ApplyDamage(void* armor, void* hitInfo, uint64_t x2);

extern thread_local void* g_currentProjectile;
