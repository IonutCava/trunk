#include "Manager.h"
using namespace std;
void Manager::add(const string& name, Resource* res)
{
	_result = _resDB.insert(pair<string,Resource*>(name,res));
	if(!_result.second) (_result.first)->second = res;
	_refCounts[name] += 1;
}

void Manager::Destroy()
{
	for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++) {
		(*_resDBiter).second->unload();
		delete (*_resDBiter).second;
		(*_resDBiter).second = NULL;
	}
	_resDB.clear();
}

Resource* Manager::find(const string& name)
{
	_resDBiter = _resDB.find(name);
	if(_resDBiter != _resDB.end())
		return _resDBiter->second;
	else
		return NULL;
}

void Manager::remove(const std::string& name) { 
	Con::getInstance().printfn("Removing resource: %s. New ref count: %d",name.c_str(),_refCounts[name] - 1);
	std::string tempName = name;
	_resDBiter = _resDB.find(name);
	if(_resDBiter != _resDB.end())
	{
		if(_refCounts[name] > 1) {
			_refCounts[name] -= 1;
		}else{
			Resource* t = _resDBiter->second;
			if(t){
				if(t->unload()){
					delete t;
					t = NULL; 
					_resDB.erase(tempName);
				}
			}
		}
	}
}

void Manager::remove(Resource* res)
{
	if(res)
		if(!res->getName().empty())
			remove(res->getName());
}