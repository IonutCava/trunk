#ifndef _RESOURCE_MANAGER_H
#define _RESOURCE_MANAGER_H


#include "Managers/Manager.h" 
#include "Utility/Headers/Singleton.h" 
class Texture;
typedef Texture Texture2D;
typedef Texture TextureCubemap;

SINGLETON_BEGIN_EXT1( ResourceManager,Manager )

public:
	Resource* LoadResource(const std::string& name);

	template<class T>
	T* LoadResource(const std::string& name,bool flag = false);

protected:
	~ResourceManager() {Destroy();}

SINGLETON_END()

#endif

