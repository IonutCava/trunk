#include "Manager.h"
using namespace std;
void Manager::add(const string& name, Resource* res){
	std::pair<ResourceMap::iterator, bool > result = _resDB.insert(make_pair(name,res));
	if(!result.second){
		remove((result.first)->second);
		(result.first)->second = res;
	}
	_refCounts[name] += 1;
}

void Manager::Destroy(){
	for(ResourceMap::iterator resDBiter = _resDB.begin(); resDBiter != _resDB.end(); resDBiter++) {
		remove(resDBiter->second,true);
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

	if(res->getName().empty()){
		Console::getInstance().errorfn("ResourceManager: Trying to remove resource with invalid name!");
		return false;
	}

	string name(res->getName());
	ResourceMap::iterator resDBiter = _resDB.find(name);

	if(resDBiter != _resDB.end()){
		if(_refCounts[name] > 1 && !force) {
			_refCounts[name] -= 1;
			Console::getInstance().printfn("Removing resource: [ %s ]. New ref count: [ %d ]",name.c_str(),_refCounts[name]);
			return false;
		}else{
			Console::getInstance().printfn("Removing resource: [ %s ].",name.c_str());
			if(res->unload()){
				_resDB.erase(name);
				_refCounts.erase(name);
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