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
T* ResourceManager::LoadResource(const ResourceDescriptor& descriptor){

	T* ptr = dynamic_cast<T*>(LoadResource(descriptor.getName()));

	if(!ptr){
		ptr = new T();
		if(!ptr) return NULL;

		if(!ptr->load(descriptor.getName())) return NULL;

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}

	return ptr;
}

template<>
Terrain* ResourceManager::LoadResource<Terrain>(const ResourceDescriptor& descriptor){

	Terrain* ptr = dynamic_cast<Terrain*>(LoadResource(descriptor.getName()));

	if(!ptr){

		ptr = new Terrain();
		if(!ptr) return NULL;

		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());
		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));;
	}

	return ptr;
}

template<>
WaterPlane* ResourceManager::LoadResource<WaterPlane>(const ResourceDescriptor& descriptor){

	WaterPlane* ptr = dynamic_cast<WaterPlane*>(LoadResource(descriptor.getName()));

	if(!ptr){

		ptr = new WaterPlane();
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}

	return ptr;
}

template<>
Texture* ResourceManager::LoadResource<Texture>(const ResourceDescriptor& descriptor){

	Texture* ptr = dynamic_cast<Texture*>(LoadResource(descriptor.getName()));
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
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Shader* ResourceManager::LoadResource<Shader>(const ResourceDescriptor& descriptor){

	Shader* ptr = dynamic_cast<Shader*>(LoadResource(descriptor.getName()));
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
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}

	return ptr;
}

template<>
Material* ResourceManager::LoadResource<Material>(const ResourceDescriptor& descriptor){

	Material* ptr = dynamic_cast<Material*>(LoadResource(descriptor.getName()));

	if(!ptr){
		ptr = new Material();
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		if(descriptor.getFlag()) ptr->skipComputeLightShaders();
		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Mesh* ResourceManager::LoadResource<Mesh>(const ResourceDescriptor& descriptor){

	Mesh* ptr = dynamic_cast<Mesh*>(LoadResource(descriptor.getName()));

	if(!ptr){
		ptr = DVDConverter::getInstance().load(descriptor.getResourceLocation());
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		if(descriptor.getFlag()) ptr->useDefaultMaterial(false);
		ptr->setName(descriptor.getName());

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Light* ResourceManager::LoadResource<Light>(const ResourceDescriptor& descriptor){

	Light* ptr = dynamic_cast<Light*>(LoadResource(descriptor.getName()));

	if(!ptr){

		ptr = new Light(descriptor.getId());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->update();

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
AudioDescriptor* ResourceManager::LoadResource<AudioDescriptor>(const ResourceDescriptor& descriptor){

	AudioDescriptor* ptr = dynamic_cast<AudioDescriptor*>(LoadResource(descriptor.getName()));

	if(!ptr){

		ptr = new AudioDescriptor();

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->isLooping() = descriptor.getFlag();

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
TerrainDescriptor* ResourceManager::LoadResource<TerrainDescriptor>(const ResourceDescriptor& descriptor){

	TerrainDescriptor* ptr = dynamic_cast<TerrainDescriptor*>(LoadResource(descriptor.getName()));

	if(!ptr){

		ptr = new TerrainDescriptor();

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Box3D* ResourceManager::LoadResource<Box3D>(const ResourceDescriptor& descriptor){
	Box3D* ptr = dynamic_cast<Box3D*>(LoadResource(descriptor.getName()));

	if(!ptr){

	  	ptr = new Box3D(1);

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->clearMaterials();
			ptr->useDefaultMaterial(false);
		}
		
		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Sphere3D* ResourceManager::LoadResource<Sphere3D>(const ResourceDescriptor& descriptor){

	Sphere3D* ptr = dynamic_cast<Sphere3D*>(LoadResource(descriptor.getName()));

	if(!ptr){

	  	ptr = new Sphere3D(1,32);

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->clearMaterials();
			ptr->useDefaultMaterial(false);
		}
		
		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Text3D* ResourceManager::LoadResource<Text3D>(const ResourceDescriptor& descriptor){

	Text3D* ptr = dynamic_cast<Text3D*>(LoadResource(descriptor.getName()));

	if(!ptr){

	  	ptr = new Text3D(descriptor.getName());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		
		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->clearMaterials();
			ptr->useDefaultMaterial(false);
		}
		
		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}
	return ptr;
}

template<>
Quad3D* ResourceManager::LoadResource<Quad3D>(const ResourceDescriptor& descriptor){

	Quad3D* ptr = dynamic_cast<Quad3D*>(LoadResource(descriptor.getName()));
	
	if(!ptr){

	  	ptr = new Quad3D(vec3(1,1,0),vec3(-1,0,0),vec3(0,-1,0),vec3(-1,-1,0));
		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		ptr->setName(descriptor.getName());

		if(descriptor.getFlag()){
			ptr->clearMaterials();
			ptr->useDefaultMaterial(false);
		}

		_resDB.insert(make_pair(descriptor.getName(),ptr));
		_refCounts.insert(make_pair(descriptor.getName(),1));
	}

	return ptr;
}

Resource* ResourceManager::LoadResource(const string& name)
{
	if(_resDB.find(name) != _resDB.end())	
	{
		Console::getInstance().printf("ResourceManager: returning resource [ %s ]. Ref count: %d\n",name.c_str(),_refCounts[name]+1);
		_refCounts[name] += 1;
		return _resDB[name];
	}
	else
	{
		Console::getInstance().printf("ResourceManager: loading resource [ %s ]\n",name.c_str());
		return NULL;
	}
}
template<class T>
void ResourceManager::remove(T *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}


template<>
void ResourceManager::remove(Terrain *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Light *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Shader *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Texture *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Material *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(AudioDescriptor *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(TerrainDescriptor *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}
template<>
void ResourceManager::remove(Object3D *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(SceneNode *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Mesh *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Sphere3D*& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Text3D *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Box3D *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}

template<>
void ResourceManager::remove(Quad3D *& res){
	if(Manager::remove(res)){
		delete res;
		res = NULL;
	}
}