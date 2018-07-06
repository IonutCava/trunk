#include "Managers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Hardware/Video/Light.h"
#include "Terrain/Terrain.h"
#include "Terrain/Water.h"
#include "Importer/DVDConverter.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Utility/Headers/BaseClasses.h"
#include "Terrain/TerrainDescriptor.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "Geometry/Predefined/Quad3D.h"
using namespace std;

U32 maxAlloc = 0;
char* zMaxFile = "";
I16 nMaxLine = 0;

void* operator new(size_t t ,char* zFile, I32 nLine){
	if (t > maxAlloc)	{
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


void operator delete(void * pxData ,char* zFile, I32 nLine){
	free(pxData);
}

ResourceManager::~ResourceManager(){
	Console::getInstance().printfn("Destroying resource manager ...");
	Console::getInstance().printfn("Deleting resource manager ...");
}

template<class T>
T* ResourceManager::loadResource(const ResourceDescriptor& descriptor){

	T* ptr = dynamic_cast<T*>(loadResource(descriptor.getName()));

	if(!ptr){
		ptr = New T();
		if(!ptr) return NULL;

		if(!ptr->load(descriptor.getName())) return NULL;

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}

	return ptr;
}

template<>
Terrain* ResourceManager::loadResource<Terrain>(const ResourceDescriptor& descriptor){

	Terrain* ptr = dynamic_cast<Terrain*>(loadResource(descriptor.getName()));

	if(!ptr){

		ptr = New Terrain();
		if(!ptr) return NULL;

		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());
		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}

	return ptr;
}

template<>
WaterPlane* ResourceManager::loadResource<WaterPlane>(const ResourceDescriptor& descriptor){

	WaterPlane* ptr = dynamic_cast<WaterPlane*>(loadResource(descriptor.getName()));

	if(!ptr){

		ptr = New WaterPlane();
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		ptr->setName(descriptor.getName());
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}

	return ptr;
}

template<>
Texture* ResourceManager::loadResource<Texture>(const ResourceDescriptor& descriptor){

	Texture* ptr = dynamic_cast<Texture*>(loadResource(descriptor.getName()));
	stringstream ss( descriptor.getResourceLocation() );
	string it;
	I8 i = 0;
	while(std::getline(ss, it, ' ')) i++;

	if(!ptr){
		if(i == 6)
			ptr = GFXDevice::getInstance().newTextureCubemap(descriptor.getFlag());
		else if (i == 1)
			ptr = GFXDevice::getInstance().newTexture2D(descriptor.getFlag());
		else{
			Console::getInstance().errorfn("TextureManager: wrong number of files for cubemap texture: [ %s ]", descriptor.getName().c_str());
			return NULL;
		}

		if(!ptr->load(descriptor.getResourceLocation())){
			Console::getInstance().errorfn("ResourceManager: could not load texture file [ %s ]", descriptor.getResourceLocation().c_str());
			return NULL;
		}

		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Shader* ResourceManager::loadResource<Shader>(const ResourceDescriptor& descriptor){

	Shader* ptr = dynamic_cast<Shader*>(loadResource(descriptor.getName()));
	if(!ptr){
		
		ptr = GFXDevice::getInstance().newShader();

		if(descriptor.getResourceLocation().compare("default") == 0)
			ptr->setResourceLocation(ParamHandler::getInstance().getParam<string>("assetsLocation") + "/shaders/");
		else
			ptr->setResourceLocation(descriptor.getResourceLocation());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}

	return ptr;
}

template<>
Material* ResourceManager::loadResource<Material>(const ResourceDescriptor& descriptor){

	Material* ptr = dynamic_cast<Material*>(loadResource(descriptor.getName()));

	if(!ptr){
		ptr = New Material();
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		if(descriptor.getFlag()) {
			ptr->setShader("");
		}
		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Mesh* ResourceManager::loadResource<Mesh>(const ResourceDescriptor& descriptor){

	Mesh* ptr = dynamic_cast<Mesh*>(loadResource(descriptor.getName()));

	if(!ptr){
		ptr = DVDConverter::getInstance().load(descriptor.getResourceLocation());
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		ptr->setName(descriptor.getName());
		ptr->setResourceLocation(descriptor.getResourceLocation());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
SubMesh* ResourceManager::loadResource<SubMesh>(const ResourceDescriptor& descriptor){
	SubMesh* ptr = dynamic_cast<SubMesh*>(loadResource(descriptor.getName()));

	if(!ptr){
		ptr = New SubMesh(descriptor.getName());
		if(!ptr->load(descriptor.getName())) return NULL;
		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Light* ResourceManager::loadResource<Light>(const ResourceDescriptor& descriptor){

	Light* ptr = dynamic_cast<Light*>(loadResource(descriptor.getName()));

	if(!ptr){

		ptr = New Light(descriptor.getId());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
AudioDescriptor* ResourceManager::loadResource<AudioDescriptor>(const ResourceDescriptor& descriptor){

	AudioDescriptor* ptr = dynamic_cast<AudioDescriptor*>(loadResource(descriptor.getName()));

	if(!ptr){

		ptr = New AudioDescriptor();

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->isLooping() = descriptor.getFlag();

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
TerrainDescriptor* ResourceManager::loadResource<TerrainDescriptor>(const ResourceDescriptor& descriptor){

	TerrainDescriptor* ptr = dynamic_cast<TerrainDescriptor*>(loadResource(descriptor.getName()));

	if(!ptr){

		ptr = New TerrainDescriptor();

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Box3D* ResourceManager::loadResource<Box3D>(const ResourceDescriptor& descriptor){
	Box3D* ptr = dynamic_cast<Box3D*>(loadResource(descriptor.getName()));

	if(!ptr){

	  	ptr = New Box3D(1);

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		
		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Sphere3D* ResourceManager::loadResource<Sphere3D>(const ResourceDescriptor& descriptor){

	Sphere3D* ptr = dynamic_cast<Sphere3D*>(loadResource(descriptor.getName()));

	if(!ptr){

	  	ptr = New Sphere3D(1,32);

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		
		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Text3D* ResourceManager::loadResource<Text3D>(const ResourceDescriptor& descriptor){

	Text3D* ptr = dynamic_cast<Text3D*>(loadResource(descriptor.getName()));

	if(!ptr){

	  	ptr = New Text3D(descriptor.getName());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		
		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		
		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}
	return ptr;
}

template<>
Quad3D* ResourceManager::loadResource<Quad3D>(const ResourceDescriptor& descriptor){

	Quad3D* ptr = dynamic_cast<Quad3D*>(loadResource(descriptor.getName()));
	
	if(!ptr){

	  	ptr = New Quad3D();
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}

		_resDB.insert(make_pair(descriptor.getName(),ptr));
	}

	return ptr;
}

Resource* ResourceManager::loadResource(const string& name)
{
	if(_resDB.find(name) != _resDB.end()){
		_resDB[name]->createCopy(); 
		Console::getInstance().printf("ResourceManager: returning resource [ %s ]. Ref count: %d\n",name.c_str(),_resDB[name]->getRefCount());
		return _resDB[name];
	}else{
		Console::getInstance().printf("ResourceManager: loading resource [ %s ]\n",name.c_str());
		return NULL;
	}
}

template <typename T>
void ResourceManager::removeResource(T*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(SceneNode*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Shader*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Texture*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Object3D*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Sphere3D*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Quad3D*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Box3D*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(Text3D*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}
template <>
void ResourceManager::removeResource(Mesh*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(SubMesh*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}
template <>
void ResourceManager::removeResource(Material*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(AudioDescriptor*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}

template <>
void ResourceManager::removeResource(TerrainDescriptor*& res,bool force){
	if(Manager::remove(res,force)){
		delete res;	
		res = NULL;
		assert(!res);
	}
}