#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {

TerrainDescriptor* ImplResourceLoader<TerrainDescriptor>::operator()() {
    TerrainDescriptor* ptr = MemoryManager_NEW TerrainDescriptor();
    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(TerrainDescriptor)
};