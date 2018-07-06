#include "Core/Resources/Headers/ResourceLoader.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"
#include "Managers/Headers/SceneManager.h"

Terrain* ImplResourceLoader<Terrain>::operator()() {

	Terrain* ptr = New Terrain();

	assert(ptr != NULL);
	if(!load(ptr,_descriptor.getName())) return NULL;

	return ptr;
}

template<>
bool ImplResourceLoader<Terrain>::load(Terrain* const res, const std::string& name) {

	std::vector<TerrainDescriptor*>& terrains = GET_ACTIVE_SCENE()->getTerrainInfoArray();
	PRINT_FN("Loading terrain [ %s ]",name.c_str());

	TerrainDescriptor* terrain = NULL;
	for(U8 i = 0; i < terrains.size(); i++)
		if(name.compare(terrains[i]->getVariable("terrainName")) == 0){
			terrain = terrains[i];
			break;
		}
	if(!terrain) return false;


	ResourceDescriptor terrainMaterial("terrainMaterial");
	res->setMaterial(CreateResource<Material>(terrainMaterial));
	res->getMaterial()->setDiffuse(vec4<F32>(1.0f, 1.0f, 1.0f, 1.0f));
	res->getMaterial()->setSpecular(vec4<F32>(1.0f, 1.0f, 1.0f, 1.0f));
	res->getMaterial()->setShininess(50.0f);
	res->getMaterial()->setShaderProgram("terrain");

	ResourceDescriptor textureNormalMap("Terrain Normal Map");
	textureNormalMap.setResourceLocation(terrain->getVariable("normalMap"));
	ResourceDescriptor textureRedTexture("Terrain Red Texture");
	textureRedTexture.setResourceLocation(terrain->getVariable("redTexture"));
	ResourceDescriptor textureGreenTexture("Terrain Green Texture");
	textureGreenTexture.setResourceLocation(terrain->getVariable("greenTexture"));
	ResourceDescriptor textureBlueTexture("Terrain Blue Texture");
	textureBlueTexture.setResourceLocation(terrain->getVariable("blueTexture"));
	std::string alphaTextureFile = terrain->getVariable("alphaTexture");
	ResourceDescriptor textureWaterCaustics("Terrain Water Caustics");
	textureWaterCaustics.setResourceLocation(terrain->getVariable("waterCaustics"));
	ResourceDescriptor textureTextureMap("Terrain Texture Map");
	textureTextureMap.setResourceLocation(terrain->getVariable("textureMap"));

	res->addTexture(Terrain::TERRAIN_TEXTURE_DIFFUSE, CreateResource<Texture>(textureTextureMap));
	res->addTexture(Terrain::TERRAIN_TEXTURE_NORMALMAP, CreateResource<Texture>(textureNormalMap));
	res->addTexture(Terrain::TERRAIN_TEXTURE_CAUSTICS, CreateResource<Texture>(textureWaterCaustics));
	res->addTexture(Terrain::TERRAIN_TEXTURE_RED, CreateResource<Texture>(textureRedTexture));
	res->addTexture(Terrain::TERRAIN_TEXTURE_GREEN, CreateResource<Texture>(textureGreenTexture));
	res->addTexture(Terrain::TERRAIN_TEXTURE_BLUE, CreateResource<Texture>(textureBlueTexture));
	if(alphaTextureFile.compare(ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/none") != 0){
		ResourceDescriptor textureAlphaTexture("Terrain Alpha Texture");
		textureAlphaTexture.setResourceLocation(alphaTextureFile);
		res->addTexture(Terrain::TERRAIN_TEXTURE_ALPHA, CreateResource<Texture>(textureAlphaTexture));
	}

	res->loadVisualResources();

	if(res->loadThreadedResources(terrain)){
		PRINT_FN("Loading Terrain [ %s ] OK", name.c_str());
		return res->setInitialData(name);
	}
	
	ERROR_FN("Error loading terrain [ %s ]", name.c_str());
	return false;
}