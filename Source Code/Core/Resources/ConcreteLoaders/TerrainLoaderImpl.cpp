#include "Core/Resources/Headers/ResourceLoader.h"

#include "Managers/Headers/SceneManager.h"

#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

template<>
Terrain* ImplResourceLoader<Terrain>::operator()() {
    Terrain* ptr = MemoryManager_NEW Terrain();
    ptr->setState(ResourceState::RES_LOADING);
    const stringImpl& name = _descriptor.getName();
    Console::printfn(Locale::get(_ID("TERRAIN_LOAD_START")), name.c_str());
    TerrainDescriptor* terrain = GET_ACTIVE_SCENE().getTerrainInfo(name);
    if (!terrain || !TerrainLoader::loadTerrain(ptr, terrain)) {
        Console::errorfn(Locale::get(_ID("ERROR_TERRAIN_LOAD")), name.c_str());
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};
