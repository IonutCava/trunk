#include "Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"

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
	Resource* value = NULL;
	if(_resDB.find(name) != _resDB.end()){
		value = _resDB[name];
		value->createCopy(); 
		D_PRINT_FN("ResourceCache: returning resource [ %s ]. Ref count: %d",name.c_str(),value->getRefCount());
	}else{
		PRINT_FN("ResourceCache: loading resource [ %s ]",name.c_str());
	}
	return value;
}