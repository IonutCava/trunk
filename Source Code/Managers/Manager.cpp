#include "Manager.h"
using namespace std;


void Manager::add(const string& name, Resource* res){
	std::pair<ResourceMap::iterator, bool > result = _resDB.insert(make_pair(name,res));
	if(!result.second){
		remove((result.first)->second);
		(result.first)->second = res;
	}
	res->incRefCount();
}

void Manager::Destroy(){
	foreach(ResourceMap::value_type iter, _resDB){
		if(iter.second){
			iter.second->unload();
			delete iter.second;
			iter.second = NULL;
		}
	}
	_resDB.clear();
}

Resource* Manager::find(const string& name){
	ResourceMap::iterator resDBiter = _resDB.find(name);
	if(resDBiter != _resDB.end())
		return resDBiter->second;
	else
		return NULL;
}

bool Manager::remove(Resource* res, bool force){
	if(!res){
		Console::getInstance().errorfn("ResourceManager: Trying to remove NULL resource!");
		return false;
	}
	string name(res->getName());
	if(name.empty()){
		Console::getInstance().errorfn("ResourceManager: Trying to remove resource with invalid name!");
		return false;
	}
	if(_resDB.find(name) != _resDB.end()){
		if(res->getRefCount() > 1 && !force) {
			res->removeCopy();
			Console::getInstance().printfn("Removing resource: [ %s ]. New ref count: [ %d ]",name.c_str(),res->getRefCount());
			return false;
		}else{
			Console::getInstance().printfn("Removing resource: [ %s ].",name.c_str());
			if(res->unload()){
				_resDB.erase(name);
				return true;
			}else{
				Console::getInstance().errorfn("Resource [ %s ] not unloaded succesfully!", name.c_str());
				return false;
			}
		}
	}

	Console::getInstance().errorfn("ResourceManager: resource [ %s ] not found in database!",name.c_str());
	return false;
}