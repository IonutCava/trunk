#include "Headers/ResourceManager.h"
#include "Utility/Headers/Guardian.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Audio/AudioDescriptor.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Environment/Water/Headers/Water.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

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
	PRINT_FN("Destroying resource manager ...");
	PRINT_FN("Deleting resource manager ...");
}

template<class T>
T* ResourceManager::loadResource(const ResourceDescriptor& descriptor){

	T* ptr = dynamic_cast<T*>(loadResource(descriptor.getName()));

	if(!ptr){
		ptr = New T();
		if(!ptr) return NULL;

		if(!ptr->load(descriptor.getName())) return NULL;

		add(descriptor.getName(),ptr);
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

		add(descriptor.getName(),ptr);
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

		add(descriptor.getName(),ptr);
	}

	return ptr;
}

template<>
Texture* ResourceManager::loadResource<Texture>(const ResourceDescriptor& descriptor){

	Texture* ptr = dynamic_cast<Texture*>(loadResource(descriptor.getName()));
	if(!ptr){
		stringstream ss( descriptor.getResourceLocation() );
		string it;
		I8 i = 0;
		while(std::getline(ss, it, ' ')) i++;

		if(i == 6)
			ptr = GFX_DEVICE.newTextureCubemap(descriptor.getFlag());
		else if (i == 1)
			ptr = GFX_DEVICE.newTexture2D(descriptor.getFlag());
		else{
			ERROR_FN("ResourceManager: wrong number of files for cubemap texture: [ %s ]", descriptor.getName().c_str());
			return NULL;
		}

		if(!ptr->load(descriptor.getResourceLocation())){
			ERROR_FN("ResourceManager: could not load texture file [ %s ]", descriptor.getResourceLocation().c_str());
			return NULL;
		}

		add(descriptor.getName(),ptr);
	}
	return ptr;
}

template<>
ShaderProgram* ResourceManager::loadResource<ShaderProgram>(const ResourceDescriptor& descriptor){

	ShaderProgram* ptr = dynamic_cast<ShaderProgram*>(loadResource(descriptor.getName()));
	if(!ptr){
		ParamHandler& par = ParamHandler::getInstance();
		ptr = GFX_DEVICE.newShaderProgram();

		if(descriptor.getResourceLocation().compare("default") == 0)
			ptr->setResourceLocation(par.getParam<string>("assetsLocation") + "/" + 
			                         par.getParam<string>("shaderLocation") + "/" );
		else
			ptr->setResourceLocation(descriptor.getResourceLocation());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;

		add(descriptor.getName(),ptr);
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
			ptr->setShaderProgram("");
		}

		add(descriptor.getName(),ptr);
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
		ptr->setResourceLocation(descriptor.getResourceLocation());

		add(descriptor.getName(),ptr);
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
		ptr->setId(descriptor.getId());
		add(descriptor.getName(),ptr);
	}
	return ptr;
}

template<>
Light* ResourceManager::loadResource<Light>(const ResourceDescriptor& descriptor){

	Light* ptr = dynamic_cast<Light*>(loadResource(descriptor.getName()));

	if(!ptr){

		//descriptor ID is not the same as light ID. This is the light's slot!!
		ptr = New Light(descriptor.getId());

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
		add(descriptor.getName(),ptr);
	}
	return ptr;
}

template<>
Trigger* ResourceManager::loadResource<Trigger>(const ResourceDescriptor& descriptor){

	Trigger* ptr = dynamic_cast<Trigger*>(loadResource(descriptor.getName()));

	if(!ptr){

		//descriptor ID is not the same as light ID. This is the light's slot!!
		ptr = New Trigger();

		if(!ptr) return NULL;
		if(!ptr->load(descriptor.getName())) return NULL;
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
		add(descriptor.getName(),ptr);
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

		add(descriptor.getName(),ptr);
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

		add(descriptor.getName(),ptr);
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


		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		
		add(descriptor.getName(),ptr);
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


		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		
		add(descriptor.getName(),ptr);
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
		

		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		
		add(descriptor.getName(),ptr);
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

		if(descriptor.getFlag()){
			ptr->useDefaultMaterial(false);
			ptr->setMaterial(NULL);
		}
		add(descriptor.getName(),ptr);
	}

	return ptr;
}

void ResourceManager::add(const std::string& name,Resource* const res){
	boost::lock_guard<boost::mutex> lock(_creationMutex);
	res->setName(name);
	_resDB.insert(make_pair(name,res));
}

Resource* ResourceManager::loadResource(const string& name){
	boost::lock_guard<boost::mutex> lock(_creationMutex);
	Resource* value = NULL;
	if(_resDB.find(name) != _resDB.end()){
		value = _resDB[name];
		value->createCopy(); 
		D_PRINT_FN("ResourceManager: returning resource [ %s ]. Ref count: %d",name.c_str(),value->getRefCount());
	}else{
		PRINT_FN("ResourceManager: loading resource [ %s ]",name.c_str());
	}
	return value;
}

