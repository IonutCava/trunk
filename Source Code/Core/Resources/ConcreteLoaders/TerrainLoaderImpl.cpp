#include "stdafx.h"

#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Headers/PlatformContext.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<Terrain>::operator()() {
    std::shared_ptr<Terrain> ptr(MemoryManager_NEW Terrain(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.resourceName()),
                                 DeleteResource(_cache));

    Console::printfn(Locale::get(_ID("TERRAIN_LOAD_START")), _descriptor.resourceName().c_str());
    const std::shared_ptr<TerrainDescriptor>& terrain = _descriptor.getPropertyDescriptor<TerrainDescriptor>();
    
    if (ptr) {
        ptr->setState(ResourceState::RES_LOADING);
    }

    if (!ptr || !TerrainLoader::loadTerrain(ptr, terrain, _context, _descriptor.getThreaded())) {
        Console::errorfn(Locale::get(_ID("ERROR_TERRAIN_LOAD")), _descriptor.resourceName().c_str());
        ptr.reset();
    }

    return ptr;
}

};
