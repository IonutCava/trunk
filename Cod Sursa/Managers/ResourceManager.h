#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "Utility/Headers/Singleton.h"
#include "resource.h"


#include "TextureManager/Texture2D.h"
#include "TextureManager/TextureCubemap.h"

#include "Importer/OBJ.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Utility/Headers/BaseClasses.h"
#include "Managers/Manager.h"
#include <unordered_map>


SINGLETON_BEGIN_EXT1( ResourceManager,Manager )

public:
	enum RES_TYPE {TEXTURE2D, TEXTURECUBEMAP, MESH, SHADER, TEMPLATE,
		           TERRAIN, MODEL, ANIMATION, SCRIPT, XMLFILE};

	Resource* LoadResource(RES_TYPE type, const string& name);

	template<class T>
	T* LoadResource(const string& name);

	inline Resource*		getResource(const std::string& name)		{assert(_resDB.find(name)!=_resDB.end()); return _resDB.find(name)->second;}
	inline Texture2D*		getTexture2D(const std::string& name)		{return (Texture2D*)getResource(name);}
	inline TextureCubemap*	getTextureCubemap(const std::string& name)	{return (TextureCubemap*)getResource(name);}
	inline ImportedModel*	getMesh(const std::string& name)			{return (ImportedModel*)getResource(name);}

protected:
	~ResourceManager() {destroy();}

SINGLETON_END()

#endif

