#include "Managers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"
#include "Hardware/Video/GFXDevice.h"
#include "Terrain/Terrain.h"
#include "TextureManager/Texture2D.h"
#include "TextureManager/TextureCubemap.h"
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
T* ResourceManager::LoadResource(const std::string& name)
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
Texture2DFlipped* ResourceManager::LoadResource<Texture2DFlipped>(const string& name)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{	
		ptr = new Texture2D();

		((Texture2D*)ptr)->loadFlipedVertically(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
	}

	return (Texture2DFlipped*)ptr;
}

template<>
Texture2D* ResourceManager::LoadResource<Texture2D>(const string& name)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
		ptr = new Texture2D();
		((Texture2D*)ptr)->load(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
	}
	return (Texture2D*)ptr;
}

template<>
TextureCubemap* ResourceManager::LoadResource<TextureCubemap>(const string& name)
{
	Resource* ptr = LoadResource(name);

	if(!ptr)
	{
		ptr = new TextureCubemap();
		((TextureCubemap*)ptr)->load(name);
		if(!ptr) return NULL;
	
		_resDB[name] = ptr;
	}
	return (TextureCubemap*)ptr;
}

template<>
Shader* ResourceManager::LoadResource<Shader>(const string& name)
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
DVDFile* ResourceManager::LoadResource<DVDFile>(const string& name)
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


