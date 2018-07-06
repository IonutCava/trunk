#include "Headers/Resource.h"
#include "Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"

ResourceCache::~ResourceCache(){
	Destroy();
	PRINT_FN("Deleting resource cache ...");
}

void ResourceCache::add(const std::string& name,Resource* const res){
	UpgradableReadLock ur_lock(_creationMutex);
	if(res == NULL) {
		ERROR_FN("ResourceCache: Resource [ %s ] creation failed!",name.c_str());
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
		D_PRINT_FN("ResourceCache: returning resource [ %s ]. Ref count: %d",name.c_str(),res->getRefCount());
	}else{
		PRINT_FN("ResourceCache: loading resource [ %s ]",name.c_str());
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
		ERROR_FN("ResourceCache: Trying to remove resource with invalid name!");
		return true; //delete pointer
	}

	if(_resDB.find(name) != _resDB.end()){
		U32 refCount = resource->getRefCount();
		if(refCount > 1 && !force) {
			resource->SubRef();
			D_PRINT_FN("Removing resource: [ %s ]. New ref count: [ %d ]",name.c_str(),refCount);
			return false; //do not delete pointer
		}else{
			PRINT_FN("Removing resource: [ %s ].",name.c_str());
			if(resource->unload()){
				return true;
			}else{
				ERROR_FN("Resource [ %s ] not unloaded succesfully!", name.c_str());
				return force;
			}
		}
	}
	
	ERROR_FN("ResourceCache: resource [ %s ] not found in database!",name.c_str());
	return force;
}