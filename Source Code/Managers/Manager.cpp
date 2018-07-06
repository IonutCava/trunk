#include "Headers/Manager.h"
using namespace std;

void Manager::add(const std::string& name, Resource* const res){
	boost::lock_guard<boost::mutex> lock(_managerMutex);
	std::pair<ResourceMap::iterator, bool > result = _resDB.insert(make_pair(name,res));
	if(!result.second){
		remove((result.first)->second);
		(result.first)->second = res;
	}
	res->incRefCount();
}

void Manager::Destroy(){
	ResourceMap::iterator& it = _resDB.begin();
	for_each(ResourceMap::value_type& it, _resDB){
		remove(it.second, true);
		delete it.second;
		it.second = NULL;
	}
	_resDB.clear();
}

Resource* const Manager::find(const string& name){

	ResourceMap::iterator resDBiter = _resDB.find(name);
	if(resDBiter != _resDB.end()){
		return resDBiter->second;
	}
	return NULL;
}

bool Manager::remove(Resource* const resource,bool force){
	if(!resource){
		Console::getInstance().errorfn("ResourceManager: Trying to remove NULL resource!");
		return false;
	}
	string name(resource->getName());
	if(name.empty()){
		Console::getInstance().errorfn("ResourceManager: Trying to remove resource with invalid name!");
		return true; //delete pointer
	}

	if(find(name)){
		if(resource->getRefCount() > 1 && !force) {
			resource->removeCopy();
			Console::getInstance().printfn("Removing resource: [ %s ]. New ref count: [ %d ]",name.c_str(),resource->getRefCount());
			return false; //do not delete pointer
		}else{
			Console::getInstance().printfn("Removing resource: [ %s ].",name.c_str());
			if(resource->unload()){
				return true;
			}else{
				Console::getInstance().errorfn("Resource [ %s ] not unloaded succesfully!", name.c_str());
				return force;
			}
		}
	}
	
	Console::getInstance().errorfn("ResourceManager: resource [ %s ] not found in database!",name.c_str());
	return force;
}