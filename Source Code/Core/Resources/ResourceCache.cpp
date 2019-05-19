#include "stdafx.h"

#include "Headers/ResourceCache.h"

#include "Environment/Terrain/Headers/TerrainLoader.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

SharedMutex ResourceLoadLock::s_hashLock;
vectorFast<size_t> ResourceLoadLock::s_loadingHashes;


void ResourceLoadLock::notifyTaskPool(PlatformContext& context) {
    context.taskPool(TaskPoolType::HIGH_PRIORITY).threadWaiting();
}

void DeleteResource::operator()(CachedResource* res) {
    WAIT_FOR_CONDITION(res->getState() == ResourceState::RES_LOADED, false);

    _context.remove(res);

    if (res && res->getType() != ResourceType::GPU_OBJECT) {
        MemoryManager::DELETE(res);
    }
}

ResourceCache::ResourceCache(PlatformContext& context)
    : PlatformContextComponent(context)
{
}

ResourceCache::~ResourceCache()
{
    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_DELETE")));
    clear();
}

void ResourceCache::printContents() const {
    UniqueLockShared w_lock(_creationMutex);
    for (ResourceMap::const_iterator it = std::cbegin(_resDB); it != std::cend(_resDB); ++it)
    {
        if (!it->second.expired())
        {
            CachedResource_ptr res = it->second.lock();
            Console::printfn(Locale::get(_ID("RESOURCE_INFO")), res->resourceName().c_str(), res->getGUID());
        }
    }
}

void ResourceCache::clear() {
    UniqueLockShared w_lock(_creationMutex);
    Console::printfn(Locale::get(_ID("STOP_RESOURCE_CACHE")));

    for (ResourceMap::iterator it = std::begin(_resDB); it != std::end(_resDB); ++it)
    {
        if (!it->second.expired()) 
        {
            CachedResource_ptr res = it->second.lock();
            if (res->getType() != ResourceType::GPU_OBJECT) {
                Console::warnfn(Locale::get(_ID("WARN_RESOURCE_LEAKED")), res->resourceName().c_str(), res->getGUID());
            }
        }
    }

    _resDB.clear();
}

void ResourceCache::add(CachedResource_wptr res) {
    if (res.expired()) {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_CACHE_LOAD_RES")));
        return;
    }

    CachedResource_ptr resource = res.lock();
    size_t hash = resource->getDescriptorHash();
    DIVIDE_ASSERT(hash != 0, "ResourceCache add error: Invalid resource hash!");

    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_ADD")), resource->resourceName().c_str(), resource->getGUID(), hash);
    UniqueLockShared w_lock(_creationMutex);
    _resDB.insert({ hash, res });
}

CachedResource_ptr ResourceCache::find(size_t descriptorHash) {
    /// Search in our resource cache
    SharedLock r_lock(_creationMutex);
    const ResourceMap::const_iterator it = _resDB.find(descriptorHash);
    if (it != std::end(_resDB)) {
        return it->second.lock();
    }

    return nullptr;
}

void ResourceCache::remove(CachedResource* resource) {
    WAIT_FOR_CONDITION(resource->getState() != ResourceState::RES_LOADING);

    size_t resourceHash = resource->getDescriptorHash();
    const stringImpl& name = resource->resourceName();
    I64 guid = resource->getGUID();

    DIVIDE_ASSERT(resourceHash != 0, Locale::get(_ID("ERROR_RESOURCE_CACHE_INVALID_NAME")));

    bool resDBEmpty = false;
    {
        SharedLock r_lock(_creationMutex);
        DIVIDE_ASSERT(_resDB.find(resourceHash) != std::end(_resDB),
                      Locale::get(_ID("ERROR_RESOURCE_CACHE_UNKNOWN_RESOURCE")));
        resDBEmpty = _resDB.empty();
    }


    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_REM_RES")), name.c_str(), resourceHash);
    resource->setState(ResourceState::RES_LOADING);
    if (resource->unload()) {
        resource->setState(ResourceState::RES_CREATED);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_REM")), name.c_str(), guid);
        resource->setState(ResourceState::RES_UNKNOWN);
    }
    
    if (resDBEmpty) {
        Console::errorfn(Locale::get(_ID("RESOURCE_CACHE_REMOVE_NO_DB")), name.c_str());
    } else {
        UniqueLockShared w_lock(_creationMutex);
        _resDB.erase(_resDB.find(resourceHash));
    }
}

};
