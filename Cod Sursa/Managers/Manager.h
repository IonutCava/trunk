#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"

class Manager
{

protected:
	std::tr1::unordered_map<std::string, Resource*> _resDB;
	std::pair<std::tr1::unordered_map<std::string, Resource*>::iterator, bool > _result;
	std::tr1::unordered_map<std::string, Resource*>::iterator _resDBiter;

public: 
	void add(const std::string& name, Resource* res);
	Resource* find(const std::string& name);
	//ToDo: Fix the following memmory leak -Ionut
	void remove(const std::string& name) {/*delete find(name);*/ _resDB.erase(name);}
	void destroy();
};

#endif