#include "Headers/ResourceCache.h"
#include "Headers/HardwareResource.h"
#include "Core/Headers/ParamHandler.h"

#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Environment/Terrain/Headers/TerrainLoader.h"

namespace Divide {

ResourceCache::ResourceCache(){
    //_loadingPool = New boost::threadpool::pool(3);
    DVDConverter::createInstance();
    TerrainLoader::createInstance();
}

ResourceCache::~ResourceCache(){
    DVDConverter::destroyInstance();
    TerrainLoader::destroyInstance();

    Destroy();
    //SAFE_DELETE(_loadingPool);
    PRINT_FN(Locale::get("RESOURCE_CACHE_DELETE"));
}

void ResourceCache::add(const stringImpl& name,Resource* const res){
    UpgradableReadLock ur_lock(_creationMutex);
    if(res == nullptr) {
        ERROR_FN(Locale::get("ERROR_RESOURCE_CACHE_LOAD_RES"),name.c_str());
        return;
    }
    UpgradeToWriteLock uw_lock(ur_lock);
    res->setName(name);
    hashAlg::insert(_resDB, hashAlg::makePair(name, res));
}

Resource* ResourceCache::loadResource(const stringImpl& name){
    ReadLock r_lock(_creationMutex);
    Resource* res = find(name);
    if(res){
        res = _resDB[name];
        res->AddRef();
        D_PRINT_FN(Locale::get("RESOURCE_CACHE_GET_RES_INC"),name.c_str(),res->getRefCount());
    }else{
        PRINT_FN(Locale::get("RESOURCE_CACHE_GET_RES"),name.c_str());
    }
    return res;
}

void ResourceCache::Destroy(){
    if(_resDB.empty())
        return;

    PRINT_FN(Locale::get("STOP_RESOURCE_CACHE"));

	for (ResourceMap::value_type& it : _resDB){
        if (removeInternal(it.second, true)) {
            SAFE_DELETE(it.second);
        }
    }
    _resDB.clear();
}

Resource* const ResourceCache::find(const stringImpl& name){
    ///Search in our resource cache
    ResourceMap::const_iterator resDBiter = _resDB.find(name);
    if(resDBiter != _resDB.end()){
        return resDBiter->second;
    }
    return nullptr;
}

bool ResourceCache::remove(Resource* const res, bool force){
    if (res == nullptr) {
        return false;
    }
    if (_resDB.empty()) {
        ERROR_FN(Locale::get("RESOURCE_CACHE_REMOVE_NO_DB"), res->getName().c_str());
        return false;
    }
    ResourceMap::iterator resDBiter = _resDB.find(res->getName());
    // If it's not in the resource database, it must've been created manually
    if (resDBiter == _resDB.end()) {
        return true;
    }
    // If we can't remove it right now ...
    if(removeInternal(res, force)){
        _resDB.erase(resDBiter);
        delete res;
        return true;
    }

    return force;
}

bool ResourceCache::removeInternal(Resource* const resource,bool force){
    assert(resource != nullptr);

    stringImpl name(resource->getName());

    if(name.empty()){
        ERROR_FN(Locale::get("ERROR_RESOURCE_CACHE_INVALID_NAME"));
        return true; //delete pointer
    }

    if(_resDB.find(name) != _resDB.end()){
        U32 refCount = resource->getRefCount();
        if(refCount > 1 && !force) {
            resource->SubRef();
            D_PRINT_FN(Locale::get("RESOURCE_CACHE_REM_RES_DEC"),name.c_str(),resource->getRefCount());
            return false; //do not delete pointer
        }else{
            PRINT_FN(Locale::get("RESOURCE_CACHE_REM_RES"),name.c_str());
            resource->setState(RES_LOADING);
            if(resource->unload()){
                resource->setState(RES_CREATED);
                return true;
            }else{
                ERROR_FN(Locale::get("ERROR_RESOURCE_REM"), name.c_str());
                resource->setState(RES_UNKNOWN);
                return force;
            }
        }
    }

    ERROR_FN(Locale::get("ERROR_RESOURCE_REM_NOT_FOUND"),name.c_str());
    return force;
}

bool ResourceCache::load(Resource* const res, const stringImpl& name) {
    assert(res != nullptr);
    res->setName(name);
    return true;
}

bool ResourceCache::loadHW(Resource* const res, const stringImpl& name){
    if(load(res,name)){
        HardwareResource* hwRes = dynamic_cast<HardwareResource* >(res);
        assert(hwRes);
        return hwRes->generateHWResource(name);
    }
    return false;
}

};