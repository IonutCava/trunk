#include "Manager.h"
using namespace std;

void Manager::add(const string& name, Resource* res)
{
	_result = _resDB.insert(pair<string,Resource*>(name,res));
	if(!_result.second) (_result.first)->second = res;
}

void Manager::destroy()
{
	for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++) {
		((*_resDBiter).second)->unload();
		delete (*_resDBiter).second;
		(*_resDBiter).second = NULL;
	}
	_resDB.clear();
}

Resource* Manager::find(const string& name)
{
	if(_resDB.find(name) != _resDB.end())
		return _resDB[name];
	else
		return NULL;
}
