#include "Core/Resources/Headers/ResourceLoader.h"

#include "Managers/Headers/SceneManager.h"

#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

Terrain* ImplResourceLoader<Terrain>::operator()() {
    Terrain* ptr = MemoryManager_NEW Terrain();
    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::DELETE( ptr );
    }

    return ptr;
}

template<>
bool ImplResourceLoader<Terrain>::load(Terrain* const res, const stringImpl& name) {
    res->setState(RES_LOADING);

    PRINT_FN(Locale::get("TERRAIN_LOAD_START"),name.c_str());
    TerrainDescriptor* terrain = GET_ACTIVE_SCENE()->getTerrainInfo(name);
    if ( !terrain ) {
        return false;
    }
    bool loadState = TerrainLoader::getInstance().loadTerrain(res, terrain);

    if ( !loadState ) {
        ERROR_FN( Locale::get( "ERROR_TERRAIN_LOAD" ), name.c_str() );
    }
    return loadState;
}

};