#include "Core/Resources/Headers/ResourceLoader.h"

#include "Managers/Headers/SceneManager.h"

#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {

template<>
Terrain* ImplResourceLoader<Terrain>::operator()() {
    Terrain* ptr = MemoryManager_NEW Terrain(_descriptor.getName());

    Console::printfn(Locale::get(_ID("TERRAIN_LOAD_START")), _descriptor.getName().c_str());
    const TerrainDescriptor* terrain = _descriptor.getPropertyDescriptor<TerrainDescriptor>();
    
    if (!ptr || !TerrainLoader::loadTerrain(ptr, terrain)) {
        Console::errorfn(Locale::get(_ID("ERROR_TERRAIN_LOAD")), _descriptor.getName().c_str());
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};
