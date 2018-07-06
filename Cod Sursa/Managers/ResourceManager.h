#ifndef _RESOURCE_MANAGER_H
#define _RESOURCE_MANAGER_H


#include "Managers/Manager.h" 
#include "Utility/Headers/Singleton.h" 

SINGLETON_BEGIN_EXT1( ResourceManager,Manager )

public:
	Resource* LoadResource(const std::string& name);

	template<class T>
	T* LoadResource(const std::string& name,bool flag = false);

protected:
	~ResourceManager() {destroy();}

SINGLETON_END()

#endif

