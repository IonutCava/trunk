#include "Managers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"

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
	Resource* ptr = LoadResource(TEMPLATE,name);
	
	ptr = new T();
	((T*)ptr)->load(name);

	if(!ptr) return NULL;

	_resDB[name] = ptr;

	return ptr;
}

template<>
Texture2D* ResourceManager::LoadResource<Texture2D>(const string& name)
{
	Resource* ptr = LoadResource(TEXTURE2D,name);

	ptr = new Texture2D();
	((Texture2D*)ptr)->load(name);

	if(!ptr) return NULL;

	_resDB[name] = ptr;

	return (Texture2D*)ptr;
}

template<>
TextureCubemap* ResourceManager::LoadResource<TextureCubemap>(const string& name)
{
	Resource* ptr = LoadResource(TEXTURECUBEMAP,name);
	ptr = new TextureCubemap();
	((TextureCubemap*)ptr)->load(name);
	if(!ptr) return NULL;
	
	_resDB[name] = ptr;

	return (TextureCubemap*)ptr;
}

template<>
Shader* ResourceManager::LoadResource<Shader>(const string& name)
{
	Resource* ptr = LoadResource(SHADER,name);


	((Shader*)ptr)->load(name);

	if(!ptr) return NULL;

	_resDB[name] = ptr;

	return (Shader*)ptr;
}
/*
template<>
ImportedModel* ResourceManager::LoadResource<ImportedModel>(const string& name)
{
	Resource* ptr = LoadResource(MODEL,name);

	((ImportedModel*)ptr)->load(name);

	if(!ptr) return NULL;

	_resDB[name] = ptr;

	return (ImportedModel*)ptr;
}
*/

Resource* ResourceManager::LoadResource(RES_TYPE type, const string& name)
{
	if(_resDB.find(name) != _resDB.end())
		return _resDB.find(name)->second;

	cout << "Resource Manager: Loading resource of type \"";
		  

	Resource* ptr = NULL;

	switch(type) {
		case TEXTURE2D: 
			ptr = new Texture2D();
			cout << "Texture 2D";
			break;
		case TEXTURECUBEMAP: 
			ptr = new TextureCubemap();
			cout << "Texture Cubemap";
			break;
		case SHADER: 
			ptr = GFXDevice::getInstance().newShader();
			cout << "Shader";
			break;
		case TERRAIN: 
			ptr = new Terrain();
			cout << "Terrain";
			break;
		case MESH: 
			ptr = new ImportedModel();
			cout << "Mesh";
			break;
	}
	cout <<  "\": \"" << name << "\"" << endl;
	if(!ptr) return NULL;
	return ptr;
}




