#include "Managers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"
#include "Hardware/Video/GFXDevice.h"
#include "Terrain/Terrain.h"
#include "Importer/DVDConverter.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Utility/Headers/BaseClasses.h"

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

		((DVDFile*)ptr)->load(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
	}
	return (DVDFile*)ptr;
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


