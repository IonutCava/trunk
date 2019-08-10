#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainLoader.h"
#include "Headers/TerrainDescriptor.h"
#include "Headers/TerrainTessellator.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

namespace Divide {

namespace {
    constexpr bool g_disableLoadFromCache = false;
    const char* g_materialFileExtension = ".mat.xml";
    stringImpl climatesLocation(U8 textureQuality) {
       CLAMP<U8>(textureQuality, 0u, 3u);

       return Paths::g_assetsLocation + Paths::g_heightmapLocation +
             (textureQuality == 3u ? Paths::g_climatesHighResLocation
                                   : textureQuality == 2u ? Paths::g_climatesMedResLocation
                                                          : Paths::g_climatesLowResLocation);
    }

    std::pair<U8, bool> findOrInsert(U8 textureQuality, vector<stringImpl>& container, const stringImpl& texture, stringImpl materialName) {
        if (!fileExists((climatesLocation(textureQuality) + "/" + materialName + "/" + texture).c_str())) {
            materialName = "std_default";
        }
        const stringImpl item = materialName + "/" + texture;
        auto it = std::find(std::cbegin(container),
                            std::cend(container),
                            item);
        if (it != std::cend(container)) {
            return std::make_pair(to_U8(std::distance(std::cbegin(container), it)), false);
        }

        container.push_back(item);
        return std::make_pair(to_U8(container.size() - 1), true);
    }
};

bool TerrainLoader::loadTerrain(Terrain_ptr terrain,
                                const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                                PlatformContext& context,
                                bool threadedLoading) {

    auto waitForReasoureTask = [&context](CachedResource_wptr res) {
        context.taskPool(TaskPoolType::HIGH_PRIORITY).threadWaiting();
    };

    const stringImpl& name = terrainDescriptor->getVariable("terrainName");
    
    stringImpl terrainLocation = Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor->getVariable("descriptor");

    Attorney::TerrainLoader::descriptor(*terrain, terrainDescriptor);

    const U8 textureQuality = to_U8(context.config().rendering.terrainTextureQuality);
    // Blend maps
    ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name);
    textureBlendMap.assetLocation(terrainLocation);
    textureBlendMap.setFlag(true);
    textureBlendMap.waitForReadyCbk(waitForReasoureTask);

    // Albedo maps
    ResourceDescriptor textureAlbedoMaps("Terrain Albedo Maps_" + name);
    textureAlbedoMaps.assetLocation(climatesLocation(textureQuality));
    textureAlbedoMaps.setFlag(true);
    textureAlbedoMaps.waitForReadyCbk(waitForReasoureTask);

    // Normals and roughness
    ResourceDescriptor textureNormalMaps("Terrain Normal Maps_" + name);
    textureNormalMaps.assetLocation(climatesLocation(textureQuality));
    textureNormalMaps.setFlag(true);
    textureNormalMaps.waitForReadyCbk(waitForReasoureTask);

    // AO and displacement
    ResourceDescriptor textureExtraMaps("Terrain Extra Maps_" + name);
    textureExtraMaps.assetLocation(climatesLocation(textureQuality));
    textureExtraMaps.setFlag(true);
    textureExtraMaps.waitForReadyCbk(waitForReasoureTask);

    //temp data
    stringImpl layerOffsetStr;
    stringImpl currentMaterial;

    U8 layerCount = terrainDescriptor->getTextureLayerCount();

    const vector<std::pair<stringImpl, TerrainTextureChannel>> channels = {
        {"red", TerrainTextureChannel::TEXTURE_RED_CHANNEL},
        {"green", TerrainTextureChannel::TEXTURE_GREEN_CHANNEL},
        {"blue", TerrainTextureChannel::TEXTURE_BLUE_CHANNEL},
        {"alpha", TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL}
    };

    const F32 albedoTilingFactor = terrainDescriptor->getVariablef("albedoTilingFactor");

    vector<stringImpl> textures[to_base(TerrainTextureType::COUNT)] = {};
    vector<stringImpl> splatTextures = {};

    size_t idxCount = layerCount * to_size(TerrainTextureChannel::COUNT);
    std::array<vector<U16>, to_base(TerrainTextureType::COUNT)> indices;
    std::array<U16, to_base(TerrainTextureType::COUNT)> offsets;
    vector<U8> channelCountPerLayer(layerCount, 0u);

    for (auto& it : indices) {
        it.resize(idxCount, 255u);
    }
    offsets.fill(0u);

    const char* textureNames[to_base(TerrainTextureType::COUNT)] = {
        "Albedo_roughness", "Normal", "Displacement"
    };

    U16 idx = 0;
    for (U8 i = 0; i < layerCount; ++i) {
        layerOffsetStr = to_stringImpl(i);
        splatTextures.push_back(terrainDescriptor->getVariable("blendMap" + layerOffsetStr));
        U8 j = 0;
        for (auto it : channels) {
            currentMaterial = terrainDescriptor->getVariable(it.first + layerOffsetStr + "_mat");
            if (currentMaterial.empty()) {
                continue;
            }

            for (U8 k = 0; k < to_base(TerrainTextureType::COUNT); ++k) {
                auto ret = findOrInsert(textureQuality, textures[k], Util::StringFormat("%s.%s", textureNames[k], k == to_base(TerrainTextureType::ALBEDO_ROUGHNESS) ? "png" : "jpg"), currentMaterial.c_str());
                indices[k][idx] = ret.first;
                if (ret.second) {
                    ++offsets[k];
                }
            }

            ++j;
            ++idx;
        }
         channelCountPerLayer[i] = j;
    }
    
    for (U8 k = 0; k < to_base(TerrainTextureType::COUNT); ++k) {
        indices[k].resize(idx);
    }

    stringImpl blendMapArray = "";
    stringImpl albedoMapArray = "";
    stringImpl normalMapArray = "";
    stringImpl extraMapArray = "";

    U32 extraMapCount = 0;
    for (const stringImpl& tex : textures[to_base(TerrainTextureType::ALBEDO_ROUGHNESS)]) {
        albedoMapArray += tex + ",";
    }
    for (const stringImpl& tex : textures[to_base(TerrainTextureType::NORMAL)]) {
        normalMapArray += tex + ",";
    }

    for (U8 i = to_U8(TerrainTextureType::DISPLACEMENT_AO); i < to_U8(TerrainTextureType::COUNT); ++i) {
        for (const stringImpl& tex : textures[i]) {
            extraMapArray += tex + ",";
            ++extraMapCount;
        }
    }

    for (const stringImpl& tex : splatTextures) {
        blendMapArray += tex + ",";
    }
    
    auto removeLastChar = [](stringImpl& strIn) {
        strIn = strIn.substr(0, strIn.length() - 1);
    };

    removeLastChar(blendMapArray);
    removeLastChar(albedoMapArray);
    removeLastChar(normalMapArray);
    removeLastChar(extraMapArray);

    SamplerDescriptor heightMapSampler = {};
    heightMapSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    heightMapSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    heightMapSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    heightMapSampler._minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    heightMapSampler._magFilter = TextureFilter::LINEAR;
    heightMapSampler._anisotropyLevel = 8;

    SamplerDescriptor blendMapSampler = {};
    blendMapSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    blendMapSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    blendMapSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    blendMapSampler._minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    blendMapSampler._magFilter = TextureFilter::LINEAR;
    blendMapSampler._anisotropyLevel = 8;

    SamplerDescriptor albedoSampler = {};
    if (false) {
        albedoSampler._wrapU = TextureWrap::MIRROR_REPEAT;
        albedoSampler._wrapV = TextureWrap::MIRROR_REPEAT;
        albedoSampler._wrapW = TextureWrap::MIRROR_REPEAT;
    }
    albedoSampler._minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    albedoSampler._magFilter = TextureFilter::LINEAR;
    albedoSampler._anisotropyLevel = 4;

    TextureDescriptor blendMapDescriptor(TextureType::TEXTURE_2D_ARRAY);
    blendMapDescriptor.setSampler(blendMapSampler);
    blendMapDescriptor._layerCount = to_U32(splatTextures.size());
    blendMapDescriptor._srgb = false;

    TextureDescriptor albedoDescriptor(TextureType::TEXTURE_2D_ARRAY);
    albedoDescriptor.setSampler(albedoSampler);
    albedoDescriptor._layerCount = to_U32(textures[to_base(TerrainTextureType::ALBEDO_ROUGHNESS)].size());
    albedoDescriptor._srgb = true;

    TextureDescriptor normalDescriptor(TextureType::TEXTURE_2D_ARRAY);
    normalDescriptor.setSampler(albedoSampler);
    normalDescriptor._layerCount = to_U32(textures[to_base(TerrainTextureType::NORMAL)].size());
    normalDescriptor._srgb = false;

    TextureDescriptor extraDescriptor(TextureType::TEXTURE_2D_ARRAY);
    extraDescriptor.setSampler(albedoSampler);
    extraDescriptor._layerCount = extraMapCount;
    extraDescriptor._srgb = false;

    textureBlendMap.assetName(blendMapArray);
    textureBlendMap.setPropertyDescriptor(blendMapDescriptor);

    textureAlbedoMaps.assetName(albedoMapArray);
    textureAlbedoMaps.setPropertyDescriptor(albedoDescriptor);

    textureNormalMaps.assetName(normalMapArray);
    textureNormalMaps.setPropertyDescriptor(normalDescriptor);

    textureExtraMaps.assetName(extraMapArray);
    textureExtraMaps.setPropertyDescriptor(extraDescriptor);

    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material_ptr terrainMaterial = CreateResource<Material>(terrain->parentResourceCache(), terrainMaterialDescriptor);
    terrainMaterial->ignoreXMLData(true);

    const vec2<U16> & terrainDimensions = terrainDescriptor->getDimensions();
    const vec2<F32> & altitudeRange = terrainDescriptor->getAltitudeRange();

    Console::d_printfn(Locale::get(_ID("TERRAIN_INFO")), terrainDimensions.width, terrainDimensions.height);

    const F32 underwaterTileScale = terrainDescriptor->getVariablef("underwaterTileScale");
    terrainMaterial->setDiffuse(FColour(DefaultColours::WHITE.rgb() * 0.5f, 1.0f));
    terrainMaterial->setSpecular(FColour(0.1f, 0.1f, 0.1f, 1.0f));
    terrainMaterial->setShininess(1.0f);
    terrainMaterial->disableTranslucency();
    terrainMaterial->setShadingMode(Material::ShadingMode::COOK_TORRANCE);

    U8 totalLayerCount = 0;
    stringImpl layerCountData = Util::StringFormat("const uint CURRENT_LAYER_COUNT[ %d ] = {", layerCount);
    stringImpl blendAmntStr = "INSERT_BLEND_AMNT_ARRAY float blendAmount[TOTAL_LAYER_COUNT] = {";
    for (U8 i = 0; i < layerCount; ++i) {
        layerCountData.append(Util::StringFormat("%d,", channelCountPerLayer[i]));
        for (U8 j = 0; j < channelCountPerLayer[i]; ++j) {
            blendAmntStr.append("0.0f,");
        }
        totalLayerCount += channelCountPerLayer[i];
    }
    removeLastChar(layerCountData);
    layerCountData.append("};");

    removeLastChar(blendAmntStr);
    blendAmntStr.append("};");

    const char* idxNames[to_base(TerrainTextureType::COUNT)] = {
        "ALBEDO_IDX", "NORMAL_IDX", "DISPLACEMENT_IDX"
    };

    std::array<stringImpl, to_base(TerrainTextureType::COUNT)> indexData = {};
    for (U8 i = 0; i < to_base(TerrainTextureType::COUNT); ++i) {
        stringImpl& dataStr = indexData[i];
        dataStr = Util::StringFormat("const uint %s[ %d ] = {", idxNames[i], indices[i].size());
        for (size_t j = 0; j < indices[i].size(); ++j) {
            dataStr.append(Util::StringFormat("%d,", indices[i][j]));
        }
        removeLastChar(dataStr);
        dataStr.append("};");
    }

    stringImpl helperTextures = terrainDescriptor->getVariable("waterCaustics") + "," +
                                terrainDescriptor->getVariable("underwaterAlbedoTexture") + "," + 
                                terrainDescriptor->getVariable("underwaterDetailTexture") + "," +
                                terrainDescriptor->getVariable("tileNoiseTexture");

    TextureDescriptor helperTexDescriptor(TextureType::TEXTURE_2D_ARRAY);
    helperTexDescriptor.setSampler(blendMapSampler);

    ResourceDescriptor textureWaterCaustics("Terrain Helper Textures_" + name);
    textureWaterCaustics.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    textureWaterCaustics.assetName(helperTextures);
    textureWaterCaustics.setPropertyDescriptor(helperTexDescriptor);
    textureWaterCaustics.waitForReadyCbk(waitForReasoureTask);

    ResourceDescriptor heightMapTexture("Terrain Heightmap_" + name);
    heightMapTexture.assetLocation(terrainLocation);
    heightMapTexture.assetName(terrainDescriptor->getVariable("heightfieldTex"));
    heightMapTexture.waitForReadyCbk(waitForReasoureTask);

    TextureDescriptor heightMapDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_SHORT);
    heightMapDescriptor.setSampler(heightMapSampler);
    heightMapTexture.setPropertyDescriptor(heightMapDescriptor);

    heightMapTexture.setFlag(true);

    terrainMaterial->setTexture(ShaderProgram::TextureUsage::TERRAIN_SPLAT, CreateResource<Texture>(terrain->parentResourceCache(), textureBlendMap));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::TERRAIN_ALBEDO_TILE, CreateResource<Texture>(terrain->parentResourceCache(), textureAlbedoMaps));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::TERRAIN_NORMAL_TILE, CreateResource<Texture>(terrain->parentResourceCache(), textureNormalMaps));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::TERRAIN_EXTRA_TILE, CreateResource<Texture>(terrain->parentResourceCache(), textureExtraMaps));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::TERRAIN_HELPER_TEXTURES, CreateResource<Texture>(terrain->parentResourceCache(), textureWaterCaustics));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::HEIGHTMAP, CreateResource<Texture>(terrain->parentResourceCache(), heightMapTexture));

    terrainMaterial->setTextureUseForDepth(ShaderProgram::TextureUsage::TERRAIN_SPLAT, true);
    terrainMaterial->setTextureUseForDepth(ShaderProgram::TextureUsage::TERRAIN_NORMAL_TILE, true);
    terrainMaterial->setTextureUseForDepth(ShaderProgram::TextureUsage::TERRAIN_HELPER_TEXTURES, true);
    terrainMaterial->setTextureUseForDepth(ShaderProgram::TextureUsage::HEIGHTMAP, true);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "terrainTess.glsl";

    ShaderModuleDescriptor tescModule = {};
    tescModule._moduleType = ShaderType::TESSELATION_CTRL;
    tescModule._sourceFile = "terrainTess.glsl";

    ShaderModuleDescriptor teseModule = {};
    teseModule._moduleType = ShaderType::TESSELATION_EVAL;
    teseModule._sourceFile = "terrainTess.glsl";

    ShaderModuleDescriptor geomModule = {};
    geomModule._moduleType = ShaderType::GEOMETRY;
    geomModule._sourceFile = "terrainTess.glsl";

    ShaderModuleDescriptor fragModule = {};
    fragModule._batchSameFile = false;
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "terrainTess.glsl";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(tescModule);
    shaderDescriptor._modules.push_back(teseModule);
    if (terrainDescriptor->wireframeDebug() != TerrainDescriptor::WireframeMode::NONE) {
        shaderDescriptor._modules.push_back(geomModule);
    }
    shaderDescriptor._modules.push_back(fragModule);

    Texture_ptr albedoTile = terrainMaterial->getTexture(ShaderProgram::TextureUsage::TERRAIN_ALBEDO_TILE).lock();
    WAIT_FOR_CONDITION(albedoTile->getState() == ResourceState::RES_LOADED);

    const U16 tileMapSize = albedoTile->getWidth();
    for (ShaderModuleDescriptor& shaderModule : shaderDescriptor._modules) {
        if (terrainDescriptor->wireframeDebug() == TerrainDescriptor::WireframeMode::EDGES) {
            shaderModule._defines.push_back(std::make_pair("TOGGLE_WIREFRAME", true));
        } else if (terrainDescriptor->wireframeDebug() == TerrainDescriptor::WireframeMode::NORMALS) {
            shaderModule._defines.push_back(std::make_pair("TOGGLE_NORMALS", true));
        }

        if (GFXDevice::getGPUVendor() == GPUVendor::AMD) {
            if (shaderModule._moduleType == (terrainDescriptor->wireframeDebug() != TerrainDescriptor::WireframeMode::NONE ? ShaderType::GEOMETRY : ShaderType::TESSELATION_EVAL)) {
                shaderModule._defines.push_back(std::make_pair("USE_CUSTOM_CLIP_PLANES", true));
            }
        } else {
            shaderModule._defines.push_back(std::make_pair("USE_CUSTOM_CLIP_PLANES", true));
        }

        shaderModule._defines.push_back(std::make_pair(Util::StringFormat("DETAIL_LEVEL %d", context.config().rendering.terrainDetailLevel), true));
        shaderModule._defines.push_back(std::make_pair("COMPUTE_TBN", true));
        shaderModule._defines.push_back(std::make_pair("TEXTURE_TILE_SIZE " + to_stringImpl(tileMapSize), true));
        shaderModule._defines.push_back(std::make_pair("ALBEDO_TILING " + to_stringImpl(albedoTilingFactor), true));
        shaderModule._defines.push_back(std::make_pair("MAX_RENDER_NODES " + to_stringImpl(Terrain::MAX_RENDER_NODES), true));
        shaderModule._defines.push_back(std::make_pair("TERRAIN_WIDTH " + to_stringImpl(terrainDimensions.width), true));
        shaderModule._defines.push_back(std::make_pair("TERRAIN_LENGTH " + to_stringImpl(terrainDimensions.height), true));
        shaderModule._defines.push_back(std::make_pair("TERRAIN_MIN_HEIGHT " + to_stringImpl(altitudeRange.x), true));
        shaderModule._defines.push_back(std::make_pair("TERRAIN_HEIGHT_RANGE " + to_stringImpl(altitudeRange.y - altitudeRange.x), true));

        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._defines.push_back(std::make_pair(blendAmntStr, true));

            if (!context.config().rendering.shadowMapping.enabled) {
                shaderModule._defines.push_back(std::make_pair("DISABLE_SHADOW_MAPPING", true));
            }

            shaderModule._defines.push_back(std::make_pair("SKIP_TEXTURES", true));
            shaderModule._defines.push_back(std::make_pair("USE_SHADING_COOK_TORRANCE", true));
            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("UNDERWATER_TILE_SCALE %d", to_I32(underwaterTileScale)), true));
            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("TOTAL_LAYER_COUNT %d", totalLayerCount), true));
            shaderModule._defines.push_back(std::make_pair(layerCountData, false));
            for (const stringImpl& str : indexData) {
                shaderModule._defines.push_back(std::make_pair(str, false));
            }

            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("MAX_TEXTURE_LAYERS %d", layerCount), true));

            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("TEXTURE_SPLAT %d", to_base(ShaderProgram::TextureUsage::TERRAIN_SPLAT)), true));
            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("TEXTURE_ALBEDO_TILE %d", to_base(ShaderProgram::TextureUsage::TERRAIN_ALBEDO_TILE)), true));
            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("TEXTURE_NORMAL_TILE %d", to_base(ShaderProgram::TextureUsage::TERRAIN_NORMAL_TILE)), true));
            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("TEXTURE_EXTRA_TILE %d", to_base(ShaderProgram::TextureUsage::TERRAIN_EXTRA_TILE)), true));
            shaderModule._defines.push_back(std::make_pair(Util::StringFormat("TEXTURE_HELPER_TEXTURES %d", to_base(ShaderProgram::TextureUsage::TERRAIN_HELPER_TEXTURES)), true));

        }
    }

    // SHADOW
    ShaderProgramDescriptor shadowDescriptor = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : shadowDescriptor._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "Shadow";
        }
        shaderModule._defines.push_back(std::make_pair("SHADOW_PASS", true));
        shaderModule._defines.push_back(std::make_pair("MAX_TESS_SCALE 32", true));
        shaderModule._defines.push_back(std::make_pair("MIN_TESS_SCALE 16", true));
    }

    ResourceDescriptor terrainShaderShadow("Terrain_Shadow-" + name);
    terrainShaderShadow.setPropertyDescriptor(shadowDescriptor);
    terrainShaderShadow.waitForReadyCbk(waitForReasoureTask);

    ShaderProgram_ptr terrainShadowShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderShadow);

    // MAIN PASS
    ShaderProgramDescriptor colourDescriptor = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : colourDescriptor._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "MainPass";
        }
		if (terrainDescriptor->parallaxMode() == TerrainDescriptor::ParallaxMode::NORMAL) {
			shaderModule._defines.push_back(std::make_pair("USE_PARALLAX_MAPPING", true));
		} else if (terrainDescriptor->parallaxMode() == TerrainDescriptor::ParallaxMode::OCCLUSION) {
			shaderModule._defines.push_back(std::make_pair("USE_PARALLAX_OCCLUSION_MAPPING", true));
		}
        shaderModule._defines.push_back(std::make_pair("MAX_TESS_SCALE 64", true));
        shaderModule._defines.push_back(std::make_pair("MIN_TESS_SCALE 8", true));
        shaderModule._defines.push_back(std::make_pair("USE_DEFERRED_NORMALS", true));
    }

    ResourceDescriptor terrainShaderColour("Terrain_Colour-" + name);
    terrainShaderColour.setPropertyDescriptor(colourDescriptor);
    terrainShaderColour.waitForReadyCbk(waitForReasoureTask);

    ShaderProgram_ptr terrainColourShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderColour);

    // PRE PASS
    ShaderProgramDescriptor prePassDescriptor = colourDescriptor;
    for (ShaderModuleDescriptor& shaderModule : prePassDescriptor._modules) {
        shaderModule._defines.push_back(std::make_pair("PRE_PASS", true));
    }

    ResourceDescriptor terrainShaderPrePass("Terrain_PrePass-" + name);
    terrainShaderPrePass.setPropertyDescriptor(prePassDescriptor);
    terrainShaderPrePass.waitForReadyCbk(waitForReasoureTask);

    ShaderProgram_ptr terrainPrePassShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderPrePass);

    // PRE PASS LQ
    ShaderProgramDescriptor prePassDescriptorLQ = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : prePassDescriptorLQ._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "";
        }
        shaderModule._defines.push_back(std::make_pair("PRE_PASS", true));
        shaderModule._defines.push_back(std::make_pair("MAX_TESS_SCALE 32", true));
        shaderModule._defines.push_back(std::make_pair("MIN_TESS_SCALE 4", true));
        shaderModule._defines.push_back(std::make_pair("LOW_QUALITY", true));
    }

    ResourceDescriptor terrainShaderPrePassLQ("Terrain_PrePass_LowQuality-" + name);
    terrainShaderPrePassLQ.setPropertyDescriptor(prePassDescriptorLQ);
    terrainShaderPrePassLQ.waitForReadyCbk(waitForReasoureTask);

    ShaderProgram_ptr terrainPrePassShaderLQ = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderPrePassLQ);

    // MAIN PASS LQ
    ShaderProgramDescriptor lowQualityDescriptor = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : lowQualityDescriptor._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "LQPass";
            shaderModule._defines.push_back(std::make_pair("WRITE_DEPTH_TO_ALPHA", true));
        }

        shaderModule._defines.push_back(std::make_pair("MAX_TESS_SCALE 32", true));
        shaderModule._defines.push_back(std::make_pair("MIN_TESS_SCALE 4", true));
        shaderModule._defines.push_back(std::make_pair("LOW_QUALITY", true));
    }

    ResourceDescriptor terrainShaderColourLQ("Terrain_Colour_LowQuality-" + name);
    terrainShaderColourLQ.setPropertyDescriptor(lowQualityDescriptor);
    terrainShaderColourLQ.waitForReadyCbk(waitForReasoureTask);

    ShaderProgram_ptr terrainColourShaderLQ = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderColourLQ);

    terrainMaterial->setShaderProgram(terrainPrePassShader, RenderPassType::PRE_PASS);
    terrainMaterial->setShaderProgram(terrainColourShader, RenderStagePass(RenderStage::DISPLAY, RenderPassType::MAIN_PASS));
    terrainMaterial->setShaderProgram(terrainColourShaderLQ, RenderStagePass(RenderStage::REFLECTION, RenderPassType::MAIN_PASS));
    terrainMaterial->setShaderProgram(terrainColourShaderLQ, RenderStagePass(RenderStage::REFRACTION, RenderPassType::MAIN_PASS));
    terrainMaterial->setShaderProgram(terrainPrePassShaderLQ, RenderStagePass(RenderStage::REFLECTION, RenderPassType::PRE_PASS));
    terrainMaterial->setShaderProgram(terrainPrePassShaderLQ, RenderStagePass(RenderStage::REFRACTION, RenderPassType::PRE_PASS));

    terrainMaterial->setShaderProgram(terrainShadowShader, RenderStagePass(RenderStage::SHADOW, RenderPassType::MAIN_PASS));

    terrain->setMaterialTpl(terrainMaterial);

    // Generate a render state
    RenderStateBlock terrainRenderState;
    terrainRenderState.setCullMode(terrainDescriptor->wireframeDebug() != TerrainDescriptor::WireframeMode::NONE ? CullMode::CW : CullMode::CCW);
    terrainRenderState.setZFunc(ComparisonFunction::EQUAL);

    // Generate a render state for drawing reflections
    RenderStateBlock terrainRenderStatePrePass = terrainRenderState;
    terrainRenderStatePrePass.setZFunc(ComparisonFunction::LEQUAL);

    RenderStateBlock terrainRenderStateReflection;
    terrainRenderStateReflection.setCullMode(terrainDescriptor->wireframeDebug() != TerrainDescriptor::WireframeMode::NONE ? CullMode::CCW : CullMode::CW);

    RenderStateBlock terrainRenderStatePrePassReflection = terrainRenderStatePrePass;
    terrainRenderStatePrePassReflection.setCullMode(terrainDescriptor->wireframeDebug() != TerrainDescriptor::WireframeMode::NONE ? CullMode::CCW : CullMode::CW);

    // Generate a shadow render state
    RenderStateBlock terrainRenderStateDepth;
    terrainRenderStateDepth.setCullMode(CullMode::CCW);
    // terrainDescDepth.setZBias(1.0f, 1.0f);
    terrainRenderStateDepth.setZFunc(ComparisonFunction::LESS);
    terrainRenderStateDepth.setColourWrites(true, true, false, false);

    terrainMaterial->setRenderStateBlock(terrainRenderState.getHash());
    terrainMaterial->setRenderStateBlock(terrainRenderStatePrePass.getHash(), RenderPassType::PRE_PASS);
    terrainMaterial->setRenderStateBlock(terrainRenderStateReflection.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::MAIN_PASS));
    terrainMaterial->setRenderStateBlock(terrainRenderStatePrePassReflection.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::PRE_PASS));
    terrainMaterial->setRenderStateBlock(terrainRenderStateDepth.getHash(), RenderStage::SHADOW);

    if (threadedLoading) {
        Start(*CreateTask(context.taskPool(TaskPoolType::HIGH_PRIORITY), [terrain, terrainDescriptor](const Task & parent) {
            loadThreadedResources(terrain, std::move(terrainDescriptor));
        }, ("TerrainLoader::loadTerrain [ " + name + " ]").c_str()));
    } else {
        loadThreadedResources(terrain, std::move(terrainDescriptor));
    }

    return true;
}

bool TerrainLoader::loadThreadedResources(Terrain_ptr terrain,
                                          const std::shared_ptr<TerrainDescriptor> terrainDescriptor) {

    stringImpl terrainMapLocation = Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor->getVariable("descriptor");
    stringImpl terrainRawFile(terrainDescriptor->getVariable("heightfield"));

    const vec2<U16>& terrainDimensions = terrainDescriptor->getDimensions();
    
    F32 minAltitude = terrainDescriptor->getAltitudeRange().x;
    F32 maxAltitude = terrainDescriptor->getAltitudeRange().y;
    BoundingBox& terrainBB = Attorney::TerrainLoader::boundingBox(*terrain);
    terrainBB.set(vec3<F32>(-terrainDimensions.x * 0.5f, minAltitude, -terrainDimensions.y * 0.5f),
                  vec3<F32>( terrainDimensions.x * 0.5f, maxAltitude,  terrainDimensions.y * 0.5f));

    const vec3<F32>& bMin = terrainBB.getMin();
    const vec3<F32>& bMax = terrainBB.getMax();

    ByteBuffer terrainCache;
    if (!g_disableLoadFromCache && terrainCache.loadFromFile(Paths::g_cacheLocation + Paths::g_terrainCacheLocation, terrainRawFile + ".cache")) {
        terrainCache >> terrain->_physicsVerts;
    }

    if (terrain->_physicsVerts.empty()) {

        vector<char> data(terrainDimensions.width * terrainDimensions.height * (sizeof(U16) / sizeof(char)), NULL);
        readFile(terrainMapLocation + "/", terrainRawFile, data, FileType::BINARY);

        constexpr F32 ushortMax = std::numeric_limits<U16>::max() + 1.0f;

        I32 terrainWidth = to_I32(terrainDimensions.x);
        I32 terrainHeight = to_I32(terrainDimensions.y);

        terrain->_physicsVerts.resize(terrainWidth * terrainHeight);

        // scale and translate all heights by half to convert from 0-255 (0-65335) to -127 - 128 (-32767 - 32768)
        F32 altitudeRange = maxAltitude - minAltitude;
        
        F32 bXRange = bMax.x - bMin.x;
        F32 bZRange = bMax.z - bMin.z;

        #pragma omp parallel for
        for (I32 height = 0; height < terrainHeight ; ++height) {
            for (I32 width = 0; width < terrainWidth; ++width) {
                I32 idxIMG = TER_COORD(width < terrainWidth - 1 ? width : width - 1,
                                       height < terrainHeight - 1 ? height : height - 1,
                                       terrainWidth);

                vec3<F32>& vertexData = terrain->_physicsVerts[TER_COORD(width, height, terrainWidth)]._position;


                F32 yOffset = 0.0f;
                U16* heightData = reinterpret_cast<U16*>(data.data());
                yOffset = (altitudeRange * (heightData[idxIMG] / ushortMax)) + minAltitude;


                //#pragma omp critical
                //Surely the id is unique and memory has also been allocated beforehand
                vertexData.set(bMin.x + (to_F32(width)) * bXRange / (terrainWidth - 1),       //X
                                yOffset,                                                      //Y
                                bMin.z + (to_F32(height)) * (bZRange) / (terrainHeight - 1)); //Z
            }
        }

        I32 offset = 2;
        #pragma omp parallel for
        for (I32 j = offset; j < terrainHeight - offset; ++j) {
            for (I32 i = offset; i < terrainWidth - offset; ++i) {
                vec3<F32> vU, vV, vUV;

                vU.set(terrain->_physicsVerts[TER_COORD(i + offset, j + 0, terrainWidth)]._position -
                       terrain->_physicsVerts[TER_COORD(i - offset, j + 0, terrainWidth)]._position);
                vV.set(terrain->_physicsVerts[TER_COORD(i + 0, j + offset, terrainWidth)]._position -
                       terrain->_physicsVerts[TER_COORD(i + 0, j - offset, terrainWidth)]._position);

                vUV.cross(vV, vU);
                vUV.normalize();
                vU = -vU;
                vU.normalize();

                //Again, not needed, I think
                //#pragma omp critical
                {
                    I32 idx = TER_COORD(i, j, terrainWidth);
                    VertexBuffer::Vertex& vert = terrain->_physicsVerts[idx];
                    vert._normal = Util::PACK_VEC3(vUV);
                    vert._tangent = Util::PACK_VEC3(vU);
                }
            }
        }
        
        for (I32 j = 0; j < offset; ++j) {
            for (I32 i = 0; i < terrainWidth; ++i) {
                I32 idx0 = TER_COORD(i, j, terrainWidth);
                I32 idx1 = TER_COORD(i, offset, terrainWidth);

                terrain->_physicsVerts[idx0]._normal = terrain->_physicsVerts[idx1]._normal;
                terrain->_physicsVerts[idx0]._tangent = terrain->_physicsVerts[idx1]._tangent;

                idx0 = TER_COORD(i, terrainHeight - 1 - j, terrainWidth);
                idx1 = TER_COORD(i, terrainHeight - 1 - offset, terrainWidth);

                terrain->_physicsVerts[idx0]._normal = terrain->_physicsVerts[idx1]._normal;
                terrain->_physicsVerts[idx0]._tangent = terrain->_physicsVerts[idx1]._tangent;
            }
        }

        for (I32 i = 0; i < offset; ++i) {
            for (I32 j = 0; j < terrainHeight; ++j) {
                I32 idx0 = TER_COORD(i, j, terrainWidth);
                I32 idx1 = TER_COORD(offset, j, terrainWidth);

                terrain->_physicsVerts[idx0]._normal = terrain->_physicsVerts[idx1]._normal;
                terrain->_physicsVerts[idx0]._tangent = terrain->_physicsVerts[idx1]._tangent;

                idx0 = TER_COORD(terrainWidth - 1 - i, j, terrainWidth);
                idx1 = TER_COORD(terrainWidth - 1 - offset, j, terrainWidth);

                terrain->_physicsVerts[idx0]._normal = terrain->_physicsVerts[idx1]._normal;
                terrain->_physicsVerts[idx0]._tangent = terrain->_physicsVerts[idx1]._tangent;
            }
        }
        terrainCache << terrain->_physicsVerts;
        terrainCache.dumpToFile(Paths::g_cacheLocation + Paths::g_terrainCacheLocation, terrainRawFile + ".cache");
    }
    
    VertexBuffer* vb = terrain->getGeometryVB();
    Attorney::TerrainTessellatorLoader::initTessellationPatch(vb);
    vb->keepData(false);
    vb->create(true);

    initializeVegetation(terrain, terrainDescriptor);
    Attorney::TerrainLoader::postBuild(*terrain);

    Console::printfn(Locale::get(_ID("TERRAIN_LOAD_END")), terrain->resourceName().c_str());
    return terrain->load();
}

void TerrainLoader::initializeVegetation(std::shared_ptr<Terrain> terrain,
                                         const std::shared_ptr<TerrainDescriptor> terrainDescriptor) {

    const U32 terrainWidth = terrainDescriptor->getDimensions().width;
    const U32 terrainHeight = terrainDescriptor->getDimensions().height;
    U32 chunkSize = to_U32(terrainDescriptor->getTessellationRange().z);
    U32 maxChunkCount = to_U32(std::ceil((terrainWidth * terrainHeight) / (chunkSize * chunkSize * 1.0f)));

    U32 maxGrassInstances = 0u, maxTreeInstances = 0u;
    Vegetation::precomputeStaticData(terrain->getGeometryVB()->context(), chunkSize, maxChunkCount, maxGrassInstances, maxTreeInstances);

    VegetationDetails& vegDetails = Attorney::TerrainLoader::vegetationDetails(*terrain);

    for (I32 i = 1; i < 5; ++i) {
        stringImpl currentMesh = terrainDescriptor->getVariable(Util::StringFormat("treeMesh%d", i));
        if (!currentMesh.empty()) {
            vegDetails.treeMeshes.push_back(currentMesh);
        }

        vegDetails.treeRotations[i - 1].set(
            terrainDescriptor->getVariablef(Util::StringFormat("treeRotationX%d", i)),
            terrainDescriptor->getVariablef(Util::StringFormat("treeRotationY%d", i)),
            terrainDescriptor->getVariablef(Util::StringFormat("treeRotationZ%d", i))
        );
    }

    vegDetails.grassScales.set(
        terrainDescriptor->getVariablef("grassScale1"),
        terrainDescriptor->getVariablef("grassScale2"),
        terrainDescriptor->getVariablef("grassScale3"),
        terrainDescriptor->getVariablef("grassScale4"));

    vegDetails.treeScales.set(
        terrainDescriptor->getVariablef("treeScale1"),
        terrainDescriptor->getVariablef("treeScale2"),
        terrainDescriptor->getVariablef("treeScale3"),
        terrainDescriptor->getVariablef("treeScale4"));
    
    
    U8 textureCount = 0;
    stringImpl textureName;
    stringImpl currentImage = terrainDescriptor->getVariable("grassBillboard1");
    if (!currentImage.empty()) {
        textureName += currentImage;
        textureCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard2");
    if (!currentImage.empty()) {
        textureName += "," + currentImage;
        textureCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard3");
    if (!currentImage.empty()) {
        textureName += "," + currentImage;
        textureCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard4");
    if (!currentImage.empty()) {
        textureName += "," + currentImage;
        textureCount++;
    }

    if (textureCount == 0){
        return;
    }

    SamplerDescriptor grassSampler = {};
    grassSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    grassSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    grassSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    grassSampler._anisotropyLevel = 8;

    TextureDescriptor grassTexDescriptor(TextureType::TEXTURE_2D_ARRAY);
    grassTexDescriptor._layerCount = textureCount;
    grassTexDescriptor.setSampler(grassSampler);
    grassTexDescriptor._srgb = true;
    grassTexDescriptor.automaticMipMapGeneration(true);

    ResourceDescriptor textureDetailMaps("Vegetation Billboards");
    textureDetailMaps.assetLocation(Paths::g_assetsLocation + terrainDescriptor->getVariable("vegetationTextureLocation"));
    textureDetailMaps.assetName(textureName);
    textureDetailMaps.setPropertyDescriptor(grassTexDescriptor);
    
    Texture_ptr grassBillboardArray = CreateResource<Texture>(terrain->parentResourceCache(), textureDetailMaps);

    vegDetails.billboardCount = textureCount;
    vegDetails.name = terrain->resourceName() + "_vegetation";
    vegDetails.parentTerrain = terrain;

    ResourceDescriptor vegetationMaterial("grassMaterial");
    Material_ptr vegMaterial = CreateResource<Material>(terrain->parentResourceCache(), vegetationMaterial);

    vegMaterial->setDiffuse(DefaultColours::WHITE);
    vegMaterial->setSpecular(FColour(0.1f, 0.1f, 0.1f, 1.0f));
    vegMaterial->setShininess(5.0f);
    vegMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    vegMaterial->setDoubleSided(false);

    ShaderModuleDescriptor vertModule = {};
    vertModule._batchSameFile = false;
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "grass.glsl";

    //vertModule._defines.push_back(std::make_pair("USE_CULL_DISTANCE", true));
    vertModule._defines.push_back(std::make_pair(Util::StringFormat("MAX_GRASS_INSTANCES %d", maxGrassInstances).c_str(), true));

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "grass.glsl";
    fragModule._defines.push_back(std::make_pair("SKIP_TEXTURES", true));
    fragModule._defines.push_back(std::make_pair(Util::StringFormat("MAX_GRASS_INSTANCES %d", maxGrassInstances).c_str(), true));
    fragModule._defines.push_back(std::make_pair("USE_DOUBLE_SIDED", true));
    fragModule._variant = "Colour";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor grassColourShader("GrassColour");
    grassColourShader.setPropertyDescriptor(shaderDescriptor);
    grassColourShader.waitForReady(false);
    ShaderProgram_ptr grassColour = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassColourShader);

    ShaderProgramDescriptor shaderOitDescriptor = shaderDescriptor;
    shaderOitDescriptor._modules.back()._defines.push_back(std::make_pair("OIT_PASS", true));
    shaderOitDescriptor._modules.back()._variant = "Colour.OIT";

    ResourceDescriptor grassColourOITShader("grassColourOIT");
    grassColourOITShader.setPropertyDescriptor(shaderOitDescriptor);
    grassColourOITShader.waitForReady(false);
    ShaderProgram_ptr grassColourOIT = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassColourOITShader);

    fragModule._variant = "PrePass";
    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor grassPrePassShader("grassPrePass");
    grassPrePassShader.setPropertyDescriptor(shaderDescriptor);
    grassPrePassShader.waitForReady(false);
    ShaderProgram_ptr grassPrePass = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassPrePassShader);

    fragModule._variant = "Shadow";
    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor grassShadowShader("grassShadow");
    grassShadowShader.setPropertyDescriptor(shaderDescriptor);
    grassShadowShader.waitForReady(false);
    ShaderProgram_ptr grassShadow = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassShadowShader);

    vegMaterial->setShaderProgram(grassColour);
    vegMaterial->setShaderProgram(grassColourOIT, RenderPassType::OIT_PASS);
    vegMaterial->setShaderProgram(grassPrePass, RenderPassType::PRE_PASS);
    vegMaterial->setShaderProgram(grassShadow, RenderStage::SHADOW);

    vegMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, grassBillboardArray);
    vegDetails.vegetationMaterialPtr = vegMaterial;


    stringImpl terrainLocation = Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor->getVariable("descriptor");

    vegDetails.grassMap.reset(new ImageTools::ImageData);
    ImageTools::ImageDataInterface::CreateImageData(terrainLocation + "/" + terrainDescriptor->getVariable("grassMap"),
                                                    0, 0, false,
                                                    *vegDetails.grassMap);

    vegDetails.treeMap.reset(new ImageTools::ImageData);
    ImageTools::ImageDataInterface::CreateImageData(terrainLocation + "/" + terrainDescriptor->getVariable("treeMap"),
                                                    0, 0, false,
                                                    *vegDetails.treeMap);
}

bool TerrainLoader::Save(const char* fileName) { return true; }

bool TerrainLoader::Load(const char* filename) { return true; }

const boost::property_tree::ptree& empty_ptree() {
    static boost::property_tree::ptree t;
    return t;
}
};
