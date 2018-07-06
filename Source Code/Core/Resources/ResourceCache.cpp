#include "stdafx.h"

#include "Headers/ResourceCache.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"
#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

SharedLock ResourceLoadLock::_hashLock;
vector<size_t> ResourceLoadLock::_loadingHashes;

void DeleteResource::operator()(CachedResource* res)
{
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

void ResourceCache::clear() {
    WriteLock w_lock(_creationMutex);
    Console::printfn(Locale::get(_ID("STOP_RESOURCE_CACHE")));

    for (ResourceMap::iterator it = std::begin(_resDB); it != std::end(_resDB); ++it)
    {
        if (!it->second.expired()) 
        {
            CachedResource_ptr res = it->second.lock();
            if (res->getType() != ResourceType::GPU_OBJECT) {
                Console::printfn(Locale::get(_ID("WARN_RESOURCE_LEAKED")), res->name().c_str(), res->getGUID());
            }
        }
    }

    _resDB.clear();
}

void ResourceCache::add(CachedResource_wptr res) {
    CachedResource_ptr resource = res.lock();

    if (resource == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_CACHE_LOAD_RES")));
        return;
    }

    size_t hash = resource->getDescriptorHash();
    DIVIDE_ASSERT(hash != 0, "ResourceCache add error: Invalid resource hash!");

    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_ADD")), resource->name().c_str(), resource->getGUID(), hash);
    WriteLock w_lock(_creationMutex);
    hashAlg::insert(_resDB, hashAlg::make_pair(hash, CachedResource_wptr(resource)));
}

CachedResource_ptr ResourceCache::loadResource(size_t descriptorHash, const stringImpl& resourceName) {
    const CachedResource_ptr& resource = find(descriptorHash);
    if (resource) {
        WAIT_FOR_CONDITION(resource->getState() == ResourceState::RES_LOADED);
    } else {
        Console::printfn(Locale::get(_ID("RESOURCE_CACHE_GET_RES")), resourceName.c_str(), descriptorHash);
    }

    return resource;
}

CachedResource_ptr ResourceCache::find(size_t descriptorHash) {
    static CachedResource_ptr emptyResource;
    /// Search in our resource cache
    ReadLock r_lock(_creationMutex);
    ResourceMap::const_iterator it = _resDB.find(descriptorHash);
    if (it != std::end(_resDB)) {
        return it->second.lock();
    }

    return emptyResource;
}

void ResourceCache::remove(CachedResource* resource) {
    WAIT_FOR_CONDITION(resource->getState() != ResourceState::RES_LOADING);

    size_t resourceHash = resource->getDescriptorHash();
    const stringImpl& name = resource->name();
    I64 guid = resource->getGUID();

    DIVIDE_ASSERT(resourceHash != 0, Locale::get(_ID("ERROR_RESOURCE_CACHE_INVALID_NAME")));

    bool resDBEmpty = false;
    {
        ReadLock r_lock(_creationMutex);
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
        WriteLock w_lock(_creationMutex);
        _resDB.erase(_resDB.find(resourceHash));
    }
}

};
