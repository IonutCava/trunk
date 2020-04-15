#include "stdafx.h"

#include "Headers/ResourceCache.h"

#include "Environment/Terrain/Headers/TerrainLoader.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

Mutex ResourceLoadLock::s_hashLock;
std::unordered_set<size_t> ResourceLoadLock::s_loadingHashes;

ResourceLoadLock::ResourceLoadLock(size_t hash, PlatformContext& context, const bool threaded)
    : _loadingHash(hash)
{
    while (true) {
        {
            UniqueLock<Mutex> w_lock(s_hashLock);
            const auto [it, success] = s_loadingHashes.insert(_loadingHash);
            if (success) {
                return;
            }
        }
        if (threaded) {
            notifyTaskPool(context);
        }
    }
}

ResourceLoadLock::~ResourceLoadLock()
{
    UniqueLock<Mutex> w_lock(s_hashLock);
    s_loadingHashes.erase(_loadingHash);
}

void ResourceLoadLock::notifyTaskPool(PlatformContext& context) {
    context.taskPool(TaskPoolType::HIGH_PRIORITY).threadWaiting();
}

void DeleteResource::operator()(CachedResource* res) {
    WAIT_FOR_CONDITION(res->getState() == ResourceState::RES_LOADED, false);

    _context->remove(res);

    if (res) {
        if (res->resourceType() != ResourceType::GPU_OBJECT) {
            MemoryManager::DELETE(res);
        } else {
            //?
        }
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
    SharedLock<SharedMutex> r_lock(_creationMutex);
    for (ResourceMap::const_iterator it = std::cbegin(_resDB); it != std::cend(_resDB); ++it) {
        assert(!it->second.expired());

        CachedResource_ptr res = it->second.lock();
        Console::printfn(Locale::get(_ID("RESOURCE_INFO")), res->resourceName().c_str(), res->getGUID());

    }
}

void ResourceCache::clear() {
    Console::printfn(Locale::get(_ID("STOP_RESOURCE_CACHE")));

    UniqueLock<SharedMutex> w_lock(_creationMutex);
    for (ResourceMap::iterator it = std::begin(_resDB); it != std::end(_resDB); ++it) {
        assert(!it->second.expired());
        
        CachedResource_ptr res = it->second.lock();
        if (res->resourceType() != ResourceType::GPU_OBJECT) {
            Console::warnfn(Locale::get(_ID("WARN_RESOURCE_LEAKED")), res->resourceName().c_str(), res->getGUID());
        }
    }

    _resDB.clear();
}

void ResourceCache::add(CachedResource_wptr res) {
    DIVIDE_ASSERT(!res.expired(), Locale::get(_ID("ERROR_RESOURCE_CACHE_LOAD_RES")));

    CachedResource_ptr resource = res.lock();
    const size_t hash = resource->descriptorHash();
    DIVIDE_ASSERT(hash != 0, "ResourceCache add error: Invalid resource hash!");

    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_ADD")), resource->resourceName().c_str(), resource->getResourceTypeName(), resource->getGUID(), hash);

    UniqueLock<SharedMutex> w_lock(_creationMutex);
    const auto ret = _resDB.emplace(hash, res);
    DIVIDE_ASSERT(ret.second, Locale::get(_ID("ERROR_RESOURCE_CACHE_LOAD_RES")));
}

CachedResource_ptr ResourceCache::find(const size_t descriptorHash) {
    /// Search in our resource cache
    SharedLock<SharedMutex> r_lock(_creationMutex);
    const ResourceMap::const_iterator it = _resDB.find(descriptorHash);
    if (it != std::end(_resDB)) {
        return it->second.lock();
    }

    return nullptr;
}

void ResourceCache::remove(CachedResource* resource) {
    WAIT_FOR_CONDITION(resource->getState() != ResourceState::RES_LOADING);
    resource->setState(ResourceState::RES_UNLOADING);

    const size_t resourceHash = resource->descriptorHash();
    const Str128& name = resource->resourceName();
    const I64 guid = resource->getGUID();

    DIVIDE_ASSERT(resourceHash != 0, Locale::get(_ID("ERROR_RESOURCE_CACHE_INVALID_NAME")));

    bool resDBEmpty = false;
    {
        SharedLock<SharedMutex> r_lock(_creationMutex);
        resDBEmpty = _resDB.empty();
        DIVIDE_ASSERT(!resDBEmpty && _resDB.find(resourceHash) != std::end(_resDB), Locale::get(_ID("ERROR_RESOURCE_CACHE_UNKNOWN_RESOURCE")));
    }


    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_REM_RES")), name.c_str(), resourceHash);
    if (!resource->unload()) {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_REM")), name.c_str(), guid);
    }

    if (resDBEmpty) {
        Console::errorfn(Locale::get(_ID("RESOURCE_CACHE_REMOVE_NO_DB")), name.c_str());
    } else {
        UniqueLock<SharedMutex> w_lock(_creationMutex);
        _resDB.erase(_resDB.find(resourceHash));
    }
    resource->setState(ResourceState::RES_UNKNOWN);
}

};
