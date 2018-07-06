#include "Core/Resources/Headers/ResourceLoader.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"
#include "Geometry/Material/Headers/Material.h"

Terrain* ImplResourceLoader<Terrain>::operator()() {
    Terrain* ptr = New Terrain();
    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }

    return ptr;
}

template<>
bool ImplResourceLoader<Terrain>::load(Terrain* const res, const std::string& name) {
    res->setState(RES_LOADING);

    vectorImpl<TerrainDescriptor*>& terrains = GET_ACTIVE_SCENE()->getTerrainInfoArray();
    PRINT_FN(Locale::get("TERRAIN_LOAD_START"),name.c_str());

    TerrainDescriptor* terrain = nullptr;
    for(U8 i = 0; i < terrains.size(); i++)
        if(name.compare(terrains[i]->getVariable("terrainName")) == 0){
            terrain = terrains[i];
            break;
        }
    if(!terrain) return false;
    SamplerDescriptor terrainDiffuseSampler;
    terrainDiffuseSampler.setWrapMode(TEXTURE_REPEAT);

    ResourceDescriptor terrainMaterial("terrainMaterial");
    res->setMaterial(CreateResource<Material>(terrainMaterial));
    res->getMaterial()->setDiffuse(DefaultColors::WHITE());
    res->getMaterial()->setAmbient(DefaultColors::WHITE() / 3);
    res->getMaterial()->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    res->getMaterial()->setShininess(20.0f);
    res->getMaterial()->addShaderDefines("COMPUTE_TBN");
    res->getMaterial()->setShaderProgram("terrain");
    res->getMaterial()->setShaderProgram("terrain.Depth",SHADOW_STAGE);
    res->getMaterial()->setShaderProgram("terrain.Depth",Z_PRE_PASS_STAGE);
    res->getMaterial()->setShaderLoadThreaded(false);

    ResourceDescriptor textureNormalMap("Terrain Normal Map");
    textureNormalMap.setResourceLocation(terrain->getVariable("normalMap"));
    ResourceDescriptor textureRedTexture("Terrain Red Texture");
    textureRedTexture.setResourceLocation(terrain->getVariable("redTexture"));
    ResourceDescriptor textureGreenTexture("Terrain Green Texture");
    textureGreenTexture.setResourceLocation(terrain->getVariable("greenTexture"));
    ResourceDescriptor textureBlueTexture("Terrain Blue Texture");
    textureBlueTexture.setResourceLocation(terrain->getVariable("blueTexture"));
    ResourceDescriptor textureWaterCaustics("Terrain Water Caustics");
    textureWaterCaustics.setResourceLocation(terrain->getVariable("waterCaustics"));
    ResourceDescriptor textureTextureMap("Terrain Texture Map");
    textureTextureMap.setResourceLocation(terrain->getVariable("textureMap"));
    textureTextureMap.setPropertyDescriptor<SamplerDescriptor>(terrainDiffuseSampler);

    res->addTexture(Terrain::TERRAIN_TEXTURE_DIFFUSE, CreateResource<Texture>(textureTextureMap));
    res->addTexture(Terrain::TERRAIN_TEXTURE_NORMALMAP, CreateResource<Texture>(textureNormalMap));
    res->addTexture(Terrain::TERRAIN_TEXTURE_CAUSTICS, CreateResource<Texture>(textureWaterCaustics));
    res->addTexture(Terrain::TERRAIN_TEXTURE_RED, CreateResource<Texture>(textureRedTexture));
    res->addTexture(Terrain::TERRAIN_TEXTURE_GREEN, CreateResource<Texture>(textureGreenTexture));
    res->addTexture(Terrain::TERRAIN_TEXTURE_BLUE, CreateResource<Texture>(textureBlueTexture));

    std::string alphaTextureFile = terrain->getVariable("alphaTexture");
    if(alphaTextureFile.compare(ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/none") != 0){
        ResourceDescriptor textureAlphaTexture("Terrain Alpha Texture");
        textureAlphaTexture.setResourceLocation(alphaTextureFile);
        res->addTexture(Terrain::TERRAIN_TEXTURE_ALPHA, CreateResource<Texture>(textureAlphaTexture));
        res->getMaterial()->addShaderDefines("USE_ALPHA_TEXTURE");
    }

    res->loadVisualResources();

    if(res->loadThreadedResources(terrain)){
        PRINT_FN(Locale::get("TERRAIN_LOAD_END"), name.c_str());
        return true;
    }

    ERROR_FN(Locale::get("ERROR_TERRAIN_LOAD"), name.c_str());
    return false;
}