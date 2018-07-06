#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

TerrainDescriptor* ImplResourceLoader<TerrainDescriptor>::operator()(){
    TerrainDescriptor* ptr = New TerrainDescriptor();
    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(TerrainDescriptor)