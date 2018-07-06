#include "Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"

#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

ResourceCache::ResourceCache()
    : _isReady(true)
{
}

ResourceCache::~ResourceCache() {
    Destroy();
    // DELETE(_loadingPool);
    Console::printfn(Locale::get(_ID("RESOURCE_CACHE_DELETE")));
}

void ResourceCache::Destroy() {
    _isReady = false;
    {
        ReadLock r_lock(_creationMutex);
        if (_resDB.empty()) {
            return;
        }
    }

    Console::printfn(Locale::get(_ID("STOP_RESOURCE_CACHE")));

    for (ResourceMap::value_type& it : _resDB) {
        WAIT_FOR_CONDITION(it.second->getState() == ResourceState::RES_LOADED ||
                           it.second->getState() == ResourceState::RES_LOADING);
        while (it.second->GetRef() > 1) {
            it.second->SubRef();
        }
        removeInternal(it.second);
    }

    {
        WriteLock w_lock(_creationMutex);
        MemoryManager::DELETE_HASHMAP(_resDB);
    }
}

void ResourceCache::add(Resource* const res) {
    assert(_isReady);

    const stringImpl& name = res->getName();
    DIVIDE_ASSERT(!name.empty(),
                  "ResourceCache add error: Invalid resource name!");
    if (res == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_CACHE_LOAD_RES")),
                         name.c_str());
        return;
    }

    WriteLock w_lock(_creationMutex);
    hashAlg::insert(_resDB, std::make_pair(_ID_RT(name), res));
}

Resource* ResourceCache::loadResource(const stringImpl& name) {
    assert(_isReady);

    Resource* resource = find(name);
    if (resource) {
        WAIT_FOR_CONDITION(resource->getState() == ResourceState::RES_LOADED);
        resource->AddRef();
    } else {
        Console::printfn(Locale::get(_ID("RESOURCE_CACHE_GET_RES")), name.c_str());
    }
    return resource;
}

Resource* const ResourceCache::find(const stringImpl& name) {
    /// Search in our resource cache
    ReadLock r_lock(_creationMutex);
    ResourceMap::const_iterator it = _resDB.find(_ID_RT(name));
    if (it != std::end(_resDB)) {
        return it->second;
    }
    return nullptr;
}

bool ResourceCache::remove(Resource* resource) {
    if (resource == nullptr) {
        return false;
    }

    bool state = true;
    stringImpl nameCpy(resource->getName());
    
    bool isEmpty = false;
    // If it's not in the resource database, it must've been created manually
    if (_isReady) {
        // We don't need to lock if we are shutting down
        ReadLock r_lock(_creationMutex);
        isEmpty = _resDB.empty();
    } else {
        isEmpty = _resDB.empty();
    }

    if (isEmpty) {
        Console::errorfn(Locale::get(_ID("RESOURCE_CACHE_REMOVE_NO_DB")),  nameCpy.c_str());
        state = false;
    } else {
        // If we can't remove it right now ...
        if (removeInternal(resource)) {
            if (_isReady) {
                _creationMutex.lock_upgrade();
            }
            _resDB.erase(_resDB.find(_ID_RT(nameCpy)));
            if (_isReady) {
                _creationMutex.unlock_upgrade();
            }

            MemoryManager::DELETE(resource);
        } else {
            state = false;
        }
    }

    return state;
}

bool ResourceCache::removeInternal(Resource* const resource) {
    assert(resource != nullptr);

    stringImpl nameCpy(resource->getName());
    DIVIDE_ASSERT(!nameCpy.empty(),
                  Locale::get(_ID("ERROR_RESOURCE_CACHE_INVALID_NAME")));
    ResourceMap::iterator it = _resDB.find(_ID_RT(nameCpy));
    DIVIDE_ASSERT(it != std::end(_resDB),
                  Locale::get(_ID("ERROR_RESOURCE_CACHE_UNKNOWN_RESOURCE")));

    U32 refCount = resource->GetRef();
    if (refCount > 1) {
        resource->SubRef();
        Console::d_printfn(Locale::get(_ID("RESOURCE_CACHE_REM_RES_DEC")),
                           nameCpy.c_str(), resource->GetRef());
        return false;  // do not delete pointer
    }

    if (refCount == 1) {
        Console::printfn(Locale::get(_ID("RESOURCE_CACHE_REM_RES")),
                         nameCpy.c_str());
        resource->setState(ResourceState::RES_LOADING);
        if (resource->unload()) {
            resource->setState(ResourceState::RES_CREATED);
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_RESOURCE_REM")),
                             nameCpy.c_str());
            resource->setState(ResourceState::RES_UNKNOWN);
        }
    }

    return true;
}

bool ResourceCache::load(Resource* const res) {
    assert(res != nullptr);
    return res->load();
}

};
