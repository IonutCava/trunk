#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"

class Manager
{

protected:
	tr1::unordered_map<string, Resource*> _resDB;
	pair<tr1::unordered_map<string, Resource*>::iterator, bool > _result;
	tr1::unordered_map<string, Resource*>::iterator _resDBiter;

public: 
	void add(const string& name, Resource* res);
	Resource* add(Resource* data, const string& name);
	Resource* find(const string& name);
	//ToDo: Fix the following memmory leak -Ionut
	void remove(const string& name) {/*delete find(name);*/ _resDB.erase(name);}
	void destroy();
};

#endif