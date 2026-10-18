#pragma once
#include <cstdint>

namespace UmbrellaLeaf::Reflect
{
    struct TypeNodeSlot
    {
        void*  typeNode    = nullptr;
        void*  lastBuilder = nullptr;
        void* (*realGetter)() = nullptr;
        void*  wrapper     = nullptr;
    };

    void* HookedTypeNode(TypeNodeSlot& slot, void* callerAddress);
    void* NewTypeNode();
    bool RegisterClass(void* typeNode, const char* name, void* baseNode, void* allocator, void* fieldRegistrar, uint32_t objectSize);
    bool RegisterObjClassFactory(const char* name, void* baseNode, void* allocator);
    void BindBuilder(void* typeNode, void* builder);
    void BindBuilder(TypeNodeSlot& slot, void* builder);
    bool SetBaseByName(void* ctx, void* builder, const char* baseName);
}
