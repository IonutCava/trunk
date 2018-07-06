#include "Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"

#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

void DeleteResource::operator()(Resource* res)
{
    _context.remove(res);
    if (res && res->getType() != ResourceType::GPU_OBJECT) {
        MemoryManager::DELETE(res);
    }
}

ResourceCache::ResourceCache(PlatformContext& context)
    : _context(context)
{
}

ResourceCache::~ResourceCache()
{
    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_DELETE")));
    clear();
    // DELETE(_loadingPool);
}


void ResourceCache::clear() {
    WriteLock w_lock(_creationMutex);
    Console::printfn(Locale::get(_ID("STOP_RESOURCE_CACHE")));
    for (ResourceMap::iterator it = std::begin(_resDB); it != std::end(_resDB);)
    {
        if (it->second.expired()) {
            it = _resDB.erase(it);
        } else {
            ++it;
        }
    }
    assert(_resDB.empty());
}

void ResourceCache::add(Resource_wptr res) {
    Resource_ptr resource = res.lock();

    if (resource == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_CACHE_LOAD_RES")));
        return;
    }

    const stringImpl& name = resource->getName();
    DIVIDE_ASSERT(!name.empty(), "ResourceCache add error: Invalid resource name!");
    WriteLock w_lock(_creationMutex);
    hashAlg::insert(_resDB, std::make_pair(_ID_RT(name), resource));
}

Resource_ptr ResourceCache::loadResource(const stringImpl& name) {
    const Resource_ptr& resource = find(name);
    if (resource) {
        WAIT_FOR_CONDITION(resource->getState() == ResourceState::RES_LOADED);
    } else {
        Console::printfn(Locale::get(_ID("RESOURCE_CACHE_GET_RES")), name.c_str());
    }

    return resource;
}

Resource_ptr ResourceCache::find(const stringImpl& name) {
    static Resource_ptr emptyResource;
    /// Search in our resource cache
    ReadLock r_lock(_creationMutex);
    ResourceMap::const_iterator it = _resDB.find(_ID_RT(name));
    if (it != std::end(_resDB)) {
        return it->second.lock();
    }

    return emptyResource;
}

void ResourceCache::remove(Resource* resource) {
    stringImpl nameCpy(resource->getName());
    DIVIDE_ASSERT(!nameCpy.empty(), Locale::get(_ID("ERROR_RESOURCE_CACHE_INVALID_NAME")));

    bool resDBEmpty = false;
    {
        ReadLock r_lock(_creationMutex);
        DIVIDE_ASSERT(_resDB.find(_ID_RT(nameCpy)) != std::end(_resDB),
                      Locale::get(_ID("ERROR_RESOURCE_CACHE_UNKNOWN_RESOURCE")));
        resDBEmpty = _resDB.empty();
    }


    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_REM_RES")), nameCpy.c_str());
    resource->setState(ResourceState::RES_LOADING);
    if (resource->unload()) {
        resource->setState(ResourceState::RES_CREATED);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_REM")), nameCpy.c_str());
        resource->setState(ResourceState::RES_UNKNOWN);
    }
    
    if (resDBEmpty) {
        Console::errorfn(Locale::get(_ID("RESOURCE_CACHE_REMOVE_NO_DB")), nameCpy.c_str());
    } else {
        WriteLock w_lock(_creationMutex);
        _resDB.erase(_resDB.find(_ID_RT(nameCpy)));
    }
}

};
