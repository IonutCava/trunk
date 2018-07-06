#include "Core/Resources/Headers/ResourceLoader.h"

#include "Managers/Headers/SceneManager.h"

#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

Terrain* ImplResourceLoader<Terrain>::operator()() {
    Terrain* ptr = MemoryManager_NEW Terrain();
    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

template <>
bool ImplResourceLoader<Terrain>::load(Terrain* const res,
                                       const stringImpl& name) {
    res->setState(ResourceState::RES_LOADING);

    Console::printfn(Locale::get("TERRAIN_LOAD_START"), name.c_str());

    TerrainDescriptor* terrain = GET_ACTIVE_SCENE().getTerrainInfo(name);

    if (!terrain || !TerrainLoader::loadTerrain(res, terrain)) {
        Console::errorfn(Locale::get("ERROR_TERRAIN_LOAD"), name.c_str());
        return false;
    }
    return true;
}
};