#include "Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"

#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

ResourceCache::ResourceCache()
{
    DVDConverter::createInstance();
}

ResourceCache::~ResourceCache() {
    DVDConverter::destroyInstance();

    Destroy();
    // DELETE(_loadingPool);
    Console::printfn(Locale::get("RESOURCE_CACHE_DELETE"));
}

void ResourceCache::Destroy() {
    WriteLock w_lock(_creationMutex);
    if (_resDB.empty()) {
        return;
    }

    Console::printfn(Locale::get("STOP_RESOURCE_CACHE"));

    for (ResourceMap::value_type& it : _resDB) {
        while (it.second->GetRef() > 1) {
            it.second->SubRef();
        }
        removeInternal(it.second);
    }

    MemoryManager::DELETE_HASHMAP(_resDB);
}

void ResourceCache::add(const stringImpl& name, Resource* const res) {
    DIVIDE_ASSERT(!name.empty(),
                  "ResourceCache add error: Invalid resource name!");
    if (res == nullptr) {
        Console::errorfn(Locale::get("ERROR_RESOURCE_CACHE_LOAD_RES"),
                         name.c_str());
        return;
    }
    res->setName(name);
    WriteLock w_lock(_creationMutex);
    hashAlg::insert(_resDB, std::make_pair(name, res));
}

Resource* ResourceCache::loadResource(const stringImpl& name) {
    Resource* resource = find(name);
    if (resource) {
        cloneResource<Resource>(resource);
    } else {
        Console::printfn(Locale::get("RESOURCE_CACHE_GET_RES"), name.c_str());
    }
    return resource;
}

Resource* const ResourceCache::find(const stringImpl& name) {
    /// Search in our resource cache
    ReadLock r_lock(_creationMutex);
    ResourceMap::const_iterator it = _resDB.find(name);
    if (it != std::end(_resDB)) {
        return it->second;
    }
    return nullptr;
}

bool ResourceCache::remove(Resource* resource) {
    if (resource == nullptr) {
        return false;
    }
    stringImpl nameCpy(resource->getName());
    // If it's not in the resource database, it must've been created manually
    ReadLock r_lock(_creationMutex);
    if (_resDB.empty()) {
        Console::errorfn(Locale::get("RESOURCE_CACHE_REMOVE_NO_DB"),
                         nameCpy.c_str());
        return false;
    }
    r_lock.unlock();
    // If we can't remove it right now ...
    if (removeInternal(resource)) {
        WriteLock w_lock(_creationMutex);
        _resDB.erase(_resDB.find(nameCpy));
        w_lock.unlock();
        MemoryManager::DELETE(resource);
    } else {
        return false;
    }

    return true;
}

bool ResourceCache::removeInternal(Resource* const resource) {
    assert(resource != nullptr);

    stringImpl nameCpy(resource->getName());
    DIVIDE_ASSERT(!nameCpy.empty(),
                  Locale::get("ERROR_RESOURCE_CACHE_INVALID_NAME"));
    ResourceMap::iterator it = _resDB.find(nameCpy);
    DIVIDE_ASSERT(it != std::end(_resDB),
                  Locale::get("ERROR_RESOURCE_CACHE_UNKNOWN_RESOURCE"));

    U32 refCount = resource->GetRef();
    if (refCount > 1) {
        resource->SubRef();
        Console::d_printfn(Locale::get("RESOURCE_CACHE_REM_RES_DEC"),
                           nameCpy.c_str(), resource->GetRef());
        return false;  // do not delete pointer
    }

    if (refCount == 1) {
        Console::printfn(Locale::get("RESOURCE_CACHE_REM_RES"),
                         nameCpy.c_str());
        resource->setState(ResourceState::RES_LOADING);
        if (resource->unload()) {
            resource->setState(ResourceState::RES_CREATED);
        } else {
            Console::errorfn(Locale::get("ERROR_RESOURCE_REM"),
                             nameCpy.c_str());
            resource->setState(ResourceState::RES_UNKNOWN);
        }
    }

    return true;
}

bool ResourceCache::load(Resource* const res, const stringImpl& name) {
    assert(res != nullptr);
    res->setName(name);
    return true;
}

bool ResourceCache::loadHW(Resource* const res, const stringImpl& name) {
    if (load(res, name)) {
        HardwareResource* hwRes = dynamic_cast<HardwareResource*>(res);
        assert(hwRes);
        return hwRes->generateHWResource(name);
    }
    return false;
}
};
