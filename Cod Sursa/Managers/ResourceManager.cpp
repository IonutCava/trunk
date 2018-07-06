#include "Managers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"
#include "Hardware/Video/GFXDevice.h"
#include "Terrain/Terrain.h"
#include "Importer/DVDConverter.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Utility/Headers/BaseClasses.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "Geometry/Predefined/Quad3D.h"

U32 maxAlloc = 0;
char* zMaxFile = "";
int nMaxLine = 0;
void* operator new(size_t t ,char* zFile, int nLine)
{
	if (t > maxAlloc)
	{
		maxAlloc = t;
		zMaxFile = zFile;
		nMaxLine = nLine;
	}

	if(Guardian::getInstance().myfile.is_open())
		Guardian::getInstance().myfile << "[ "<< ((GETTIME() > 0 && GETTIME() <= 100000) ? GETTIME() : 0) 
									   << " ] : New allocation: " << t << " IN: \""
									   << zFile << "\" at line: " << nLine << "." << endl
									   << "\t Max Allocation: ["  << maxAlloc << "] in file: \""
									   << zMaxFile << "\" at line: " << nMaxLine
									   << endl << endl;
	return malloc(t);
}

void* operator new(size_t t)
{
	return malloc(t);
}

void operator delete(void * pxData ,char* zFile, int nLine)
{
	free(pxData);
}

void operator delete(void *pxData)
{
	free(pxData);
}

template<class T>
T* ResourceManager::LoadResource(const std::string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
		ptr = new T();
		((T*)ptr)->load(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
	}

	return (T*)ptr;
}

template<>
Texture* ResourceManager::LoadResource<Texture>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	std::stringstream ss( name );
	std::string it;
	int i = 0;
	while(std::getline(ss, it, ' ')) i++;

	if(!ptr)
	{
		if(i == 6)
			ptr = GFXDevice::getInstance().newTextureCubemap(flag);
		else if (i == 1)
			ptr = GFXDevice::getInstance().newTexture2D(flag);
		else
		{
			std::cout << "TextureManager ERROR: wrong number of files for cubemap texture: " << name << std::endl;
			return NULL;
		}

		((Texture*)ptr)->load(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
	}
	return (Texture*)ptr;
}

template<>
Shader* ResourceManager::LoadResource<Shader>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{

		ptr = GFXDevice::getInstance().newShader();
		((Shader*)ptr)->load(name);
	
		if(!ptr) return NULL;

		_resDB[name] = ptr;
	}

	return (Shader*)ptr;
}

template<>
DVDFile* ResourceManager::LoadResource<DVDFile>(const string& name,bool flag)
{

	Resource* ptr = LoadResource(name);

	if(!ptr)
	{
		ptr = new DVDFile();
		if(!((DVDFile*)ptr)->load(name) || !ptr) return NULL;

		_resDB[name] = ptr;
	}
	return (DVDFile*)ptr;
}

template<>
Object3D* ResourceManager::LoadResource<Object3D>(const string& name,bool flag)
{

	Resource* ptr = LoadResource(name);

	if(!ptr)
	{
		if(name.compare("Box3D") == 0)
			ptr = new Box3D(1);
		else if(name.compare("Sphere3D") == 0)
			ptr = new Sphere3D(1,32);
		else if(name.compare("Quad3D") == 0)
			ptr = new Quad3D(vec3(1,1,0),vec3(-1,0,0),vec3(0,-1,0),vec3(-1,-1,0));
		else if(name.compare("Text3D") == 0)
			ptr = new Text3D("");
		else
			ptr = NULL;

		if(!ptr) return NULL;
		_resDB[name] = ptr;
	}
	return (Object3D*)ptr;
}
Resource* ResourceManager::LoadResource(const string& name)
{
	if(_resDB.find(name) != _resDB.end())	
	{
		cout << "ResourceManager: returning resource [ " << name << " ]" << endl;
		return _resDB[name];
	}
	else
	{
		cout << "ResourceManager: loading resource [ " << name << " ]" << endl;
		return NULL;
	}
}


