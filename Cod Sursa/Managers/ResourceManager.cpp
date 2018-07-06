#include "Managers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Terrain/Terrain.h"
#include "Importer/DVDConverter.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Utility/Headers/BaseClasses.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "Geometry/Predefined/Quad3D.h"
using namespace std;

U32 maxAlloc = 0;
char* zMaxFile = "";
I16 nMaxLine = 0;

void* operator new(size_t t ,char* zFile, int nLine)
{
	if (t > maxAlloc)
	{
		maxAlloc = t;
		zMaxFile = zFile;
		nMaxLine = nLine;
	}

	if(Guardian::getInstance().myfile.is_open())
		Guardian::getInstance().myfile << "[ "<< GETTIME()
									   << " ] : New allocation: " << t << " IN: \""
									   << zFile << "\" at line: " << nLine << "." << endl
									   << "\t Max Allocation: ["  << maxAlloc << "] in file: \""
									   << zMaxFile << "\" at line: " << nMaxLine
									   << endl << endl;
	return malloc(t);
}


void operator delete(void * pxData ,char* zFile, int nLine)
{
	free(pxData);
}

/****************************************************************************************/
/*@Param: name - the unique name of the resource to be loaded. Can be anything.         */
/*@Param: flag - different uses depending on the resource derived class:                */
/*			   - Textures are flipped vertically if flag == true;                       */
/*             - Object3D based classes skip default material assigment if flag == true */          
/****************************************************************************************/
template<class T>
T* ResourceManager::LoadResource(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
		ptr = new T();
		dynamic_cast<T*>(ptr)->load(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}

	return dynamic_cast<T*>(ptr);
}

template<>
Texture* ResourceManager::LoadResource<Texture>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	stringstream ss( name );
	string it;
	I8 i = 0;
	while(std::getline(ss, it, ' ')) i++;

	if(!ptr)
	{
		if(i == 6)
			ptr = GFXDevice::getInstance().newTextureCubemap(flag);
		else if (i == 1)
			ptr = GFXDevice::getInstance().newTexture2D(flag);
		else
		{
			Con::getInstance().errorfn("TextureManager ERROR: wrong number of files for cubemap texture: [ %s ]", name.c_str());
			return NULL;
		}

		dynamic_cast<Texture*>(ptr)->load(name);

		if(!ptr) return NULL;

		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<Texture*>(ptr);
}

template<>
Shader* ResourceManager::LoadResource<Shader>(const string& name,bool flag)
{
	//ToDo: add a counter for shaders so that it get's deleted when no more objects need it anymore - Ionut
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{

		ptr = GFXDevice::getInstance().newShader();
		dynamic_cast<Shader*>(ptr)->load(name);
	
		if(!ptr) return NULL;

		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}

	return dynamic_cast<Shader*>(ptr);
}

template<>
Mesh* ResourceManager::LoadResource<Mesh>(const string& name,bool flag)
{

	Resource* ptr = LoadResource(name);

	if(!ptr)
	{
		ptr = DVDConverter::getInstance().load(name);
		if(!dynamic_cast<Mesh*>(ptr)->load(name) || !ptr) return NULL;

		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<Mesh*>(ptr);
}

template<>
AudioDescriptor* ResourceManager::LoadResource<AudioDescriptor>(const string& name, bool flag)
{
	Resource* ptr = LoadResource(name);

	if(!ptr)
	{
		ptr = new AudioDescriptor();
		if(!dynamic_cast<AudioDescriptor*>(ptr)->load(name) || !ptr) return NULL;
		dynamic_cast<AudioDescriptor*>(ptr)->isLooping() = flag;
		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<AudioDescriptor*>(ptr);
}

template<>
Box3D* ResourceManager::LoadResource<Box3D>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
	  	ptr = new Box3D(1);
		if(!dynamic_cast<Box3D*>(ptr)->load(name) || !ptr) return NULL;
		if(flag) dynamic_cast<Box3D*>(ptr)->clearMaterials();
		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<Box3D*>(ptr);
}

template<>
Sphere3D* ResourceManager::LoadResource<Sphere3D>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
	  	ptr = new Sphere3D(1,32);
		if(!dynamic_cast<Sphere3D*>(ptr)->load(name) || !ptr) return NULL;
		if(flag) dynamic_cast<Sphere3D*>(ptr)->clearMaterials();
		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<Sphere3D*>(ptr);
}

template<>
Text3D* ResourceManager::LoadResource<Text3D>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
	  	ptr = new Text3D(name);
		if(!dynamic_cast<Text3D*>(ptr)->load(name) || !ptr) return NULL;
		if(flag) dynamic_cast<Text3D*>(ptr)->clearMaterials();
		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<Text3D*>(ptr);
}

template<>
Quad3D* ResourceManager::LoadResource<Quad3D>(const string& name,bool flag)
{
	Resource* ptr = LoadResource(name);
	if(!ptr)
	{
	  	ptr = new Quad3D(vec3(1,1,0),vec3(-1,0,0),vec3(0,-1,0),vec3(-1,-1,0));
		if(!dynamic_cast<Quad3D*>(ptr)->load(name) || !ptr) return NULL;
		if(flag) dynamic_cast<Quad3D*>(ptr)->clearMaterials();
		_resDB[name] = ptr;
		_refCounts[name] = 1;
	}
	return dynamic_cast<Quad3D*>(ptr);
}

Resource* ResourceManager::LoadResource(const string& name)
{
	if(_resDB.find(name) != _resDB.end())	
	{
		Con::getInstance().printf("ResourceManager: returning resource [ %s ]. Ref count: %d\n",name.c_str(),_refCounts[name]+1);
		_refCounts[name] += 1;
		return _resDB[name];
	}
	else
	{
		Con::getInstance().printf("ResourceManager: loading resource [ %s ]\n",name.c_str());
		return NULL;
	}
}


