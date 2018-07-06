#include "Headers/ResourceCache.h"
#include "Headers/HardwareResource.h"
#include "Core/Headers/ParamHandler.h"

ResourceCache::ResourceCache(){
	_loadingPool = New boost::threadpool::pool(3);
}

ResourceCache::~ResourceCache(){
	Destroy();
	SAFE_DELETE(_loadingPool);
	PRINT_FN(Locale::get("RESOURCE_CACHE_DELETE"));
}

void ResourceCache::add(const std::string& name,Resource* const res){
	UpgradableReadLock ur_lock(_creationMutex);
	if(res == NULL) {
		ERROR_FN(Locale::get("ERROR_RESOURCE_CACHE_LOAD_RES"),name.c_str());
		return;
	}
	UpgradeToWriteLock uw_lock(ur_lock);
	res->setName(name);
	_resDB.insert(make_pair(name,res));
}

Resource* ResourceCache::loadResource(const std::string& name){
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
	for_each(ResourceMap::value_type& it, _resDB){
		remove(it.second, true);
		SAFE_DELETE(it.second);
	}
	_resDB.clear();
}

Resource* const ResourceCache::find(const std::string& name){
	///Search in our resource cache
	ResourceMap::iterator resDBiter = _resDB.find(name);
	if(resDBiter != _resDB.end()){
		return resDBiter->second;
	}
	return NULL;
}


bool ResourceCache::scheduleDeletion(Resource* const resource,bool force){
	assert(resource != NULL);
	ResourceMap::iterator resDBiter = _resDB.find(resource->getName());
	 /// it's already deleted. Double-deletion should be safe
	if(resDBiter == _resDB.end()) return true;
	///If we can't remove it right now ...
	if(remove(resource,force)){
		_resDB.erase(resDBiter);
		return true;
	}
	return force; 
}

bool ResourceCache::remove(Resource* const resource,bool force){

	assert(resource != NULL);

	std::string name(resource->getName());

	if(name.empty()){
		ERROR_FN(Locale::get("ERROR_RESOURCE_CACHE_INVALID_NAME"));
		return true; //delete pointer
	}

	if(_resDB.find(name) != _resDB.end()){
		U32 refCount = resource->getRefCount();
		if(refCount > 1 && !force) {
			resource->SubRef();
			D_PRINT_FN(Locale::get("RESOURCE_CACHE_REM_RES_DEC"),name.c_str(),refCount);
			return false; //do not delete pointer
		}else{
			PRINT_FN(Locale::get("RESOURCE_CACHE_REM_RES"),name.c_str());
			if(resource->unload()){
				return true;
			}else{
				ERROR_FN(Locale::get("ERROR_RESOURCE_REM"), name.c_str());
				return force;
			}
		}
	}
	
	ERROR_FN(Locale::get("ERROR_RESOURCE_REM_NOT_FOUND"),name.c_str());
	return force;
}

bool ResourceCache::load(Resource* const res, const std::string& name) {
	assert(res != NULL);
	return res->setInitialData(name);
}

bool ResourceCache::loadHW(Resource* const res, const std::string& name){
	if(load(res,name)){
		HardwareResource* hwRes = dynamic_cast<HardwareResource* >(res);
		assert(hwRes);
		return hwRes->generateHWResource(name); 
	} 
	return false; 
}