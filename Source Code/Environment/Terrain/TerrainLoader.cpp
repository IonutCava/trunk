#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainLoader.h"
#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

namespace {
    constexpr bool g_disableLoadFromCache = false;

    Str256 ClimatesLocation(U8 textureQuality) {
       CLAMP<U8>(textureQuality, 0u, 3u);

       return Paths::g_assetsLocation + Paths::g_heightmapLocation +
             (textureQuality == 3u ? Paths::g_climatesHighResLocation
                                   : textureQuality == 2u ? Paths::g_climatesMedResLocation
                                                          : Paths::g_climatesLowResLocation);
    }

    std::pair<U8, bool> findOrInsert(U8 textureQuality, vectorEASTL<stringImpl>& container, const stringImpl& texture, stringImpl materialName) {
        if (!fileExists((ClimatesLocation(textureQuality) + "/" + materialName + "/" + texture).c_str())) {
            materialName = "std_default";
        }
        const stringImpl item = materialName + "/" + texture;
        const auto it = eastl::find(eastl::cbegin(container),
                                    eastl::cend(container),
                                    item);
        if (it != eastl::cend(container)) {
            return std::make_pair(to_U8(eastl::distance(eastl::cbegin(container), it)), false);
        }

        container.push_back(item);
        return std::make_pair(to_U8(container.size() - 1), true);
    }
};

bool TerrainLoader::loadTerrain(Terrain_ptr terrain,
                                const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                                PlatformContext& context,
                                bool threadedLoading) {

    const stringImpl& name = terrainDescriptor->getVariable("terrainName");
    
    stringImpl terrainLocation = Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor->getVariable("descriptor");

    Attorney::TerrainLoader::descriptor(*terrain, terrainDescriptor);
    const U8 textureQuality = to_U8(context.config().rendering.terrainTextureQuality);
    // Blend maps
    ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name);
    textureBlendMap.assetLocation(terrainLocation);
    textureBlendMap.flag(true);

    // Albedo maps and roughness
    ResourceDescriptor textureAlbedoMaps("Terrain Albedo Maps_" + name);
    textureAlbedoMaps.assetLocation(ClimatesLocation(textureQuality));
    textureAlbedoMaps.flag(true);

    // Normals
    ResourceDescriptor textureNormalMaps("Terrain Normal Maps_" + name);
    textureNormalMaps.assetLocation(ClimatesLocation(textureQuality));
    textureNormalMaps.flag(true);

    // AO and displacement
    ResourceDescriptor textureExtraMaps("Terrain Extra Maps_" + name);
    textureExtraMaps.assetLocation(ClimatesLocation(textureQuality));
    textureExtraMaps.flag(true);

    //temp data
    stringImpl layerOffsetStr;
    stringImpl currentMaterial;

    U8 layerCount = terrainDescriptor->textureLayers();

    const vectorEASTL<std::pair<stringImpl, TerrainTextureChannel>> channels = {
        {"red", TerrainTextureChannel::TEXTURE_RED_CHANNEL},
        {"green", TerrainTextureChannel::TEXTURE_GREEN_CHANNEL},
        {"blue", TerrainTextureChannel::TEXTURE_BLUE_CHANNEL},
        {"alpha", TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL}
    };

    

    vectorEASTL<stringImpl> textures[to_base(TerrainTextureType::COUNT)] = {};
    vectorEASTL<stringImpl> splatTextures = {};

    size_t idxCount = layerCount * to_size(TerrainTextureChannel::COUNT);
    std::array<vectorEASTL<U16>, to_base(TerrainTextureType::COUNT)> indices;
    std::array<U16, to_base(TerrainTextureType::COUNT)> offsets;
    vectorEASTL<U8> channelCountPerLayer(layerCount, 0u);

    for (auto& it : indices) {
        it.resize(idxCount, 255u);
    }
    offsets.fill(0u);

    const char* textureNames[to_base(TerrainTextureType::COUNT)] = {
        "Albedo_roughness", "Normal", "Displacement"
    };

    U16 idx = 0;
    for (U8 i = 0; i < layerCount; ++i) {
        layerOffsetStr = Util::to_string(i);
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

    U16 extraMapCount = 0;
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
    heightMapSampler.wrapUVW(TextureWrap::CLAMP_TO_BORDER);
    heightMapSampler.minFilter(TextureFilter::LINEAR);
    heightMapSampler.magFilter(TextureFilter::LINEAR);
    heightMapSampler.borderColour(DefaultColours::BLACK);
    heightMapSampler.anisotropyLevel(0);
    const size_t heightSamplerHash = heightMapSampler.getHash();

    SamplerDescriptor blendMapSampler = {};
    blendMapSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
    blendMapSampler.minFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    blendMapSampler.magFilter(TextureFilter::LINEAR);
    blendMapSampler.anisotropyLevel(0);
    const size_t blendMapHash = blendMapSampler.getHash();

    SamplerDescriptor albedoSampler = {};
    albedoSampler.wrapUVW(TextureWrap::REPEAT);
    albedoSampler.minFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    albedoSampler.magFilter(TextureFilter::LINEAR);
    albedoSampler.anisotropyLevel(4);
    const size_t albedoHash = albedoSampler.getHash();

    TextureDescriptor blendMapDescriptor(TextureType::TEXTURE_2D_ARRAY);
    blendMapDescriptor.layerCount(to_U16(splatTextures.size()));
    blendMapDescriptor.srgb(false);

    TextureDescriptor albedoDescriptor(TextureType::TEXTURE_2D_ARRAY);
    albedoDescriptor.layerCount(to_U16(textures[to_base(TerrainTextureType::ALBEDO_ROUGHNESS)].size()));
    albedoDescriptor.srgb(false);

    TextureDescriptor normalDescriptor(TextureType::TEXTURE_2D_ARRAY);
    normalDescriptor.layerCount(to_U16(textures[to_base(TerrainTextureType::NORMAL)].size()));
    normalDescriptor.srgb(false);

    TextureDescriptor extraDescriptor(TextureType::TEXTURE_2D_ARRAY);
    extraDescriptor.layerCount(extraMapCount);
    extraDescriptor.srgb(false);

    textureBlendMap.assetName(blendMapArray);
    textureBlendMap.propertyDescriptor(blendMapDescriptor);

    textureAlbedoMaps.assetName(albedoMapArray);
    textureAlbedoMaps.propertyDescriptor(albedoDescriptor);

    textureNormalMaps.assetName(normalMapArray);
    textureNormalMaps.propertyDescriptor(normalDescriptor);

    textureExtraMaps.assetName(extraMapArray);
    textureExtraMaps.propertyDescriptor(extraDescriptor);

    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material_ptr terrainMaterial = CreateResource<Material>(terrain->parentResourceCache(), terrainMaterialDescriptor);
    terrainMaterial->ignoreXMLData(true);

    const vec2<U16> & terrainDimensions = terrainDescriptor->dimensions();
    const vec2<F32> & altitudeRange = terrainDescriptor->altitudeRange();

    Console::d_printfn(Locale::get(_ID("TERRAIN_INFO")), terrainDimensions.width, terrainDimensions.height);

    const F32 underwaterTileScale = terrainDescriptor->getVariablef("underwaterTileScale");
    terrainMaterial->shadingMode(ShadingMode::COOK_TORRANCE);

    if (terrainDescriptor->parallaxMode() == TerrainDescriptor::ParallaxMode::NORMAL) {
        terrainMaterial->bumpMethod(BumpMethod::PARALLAX);
    } else if (terrainDescriptor->parallaxMode() == TerrainDescriptor::ParallaxMode::OCCLUSION) {
        terrainMaterial->bumpMethod(BumpMethod::PARALLAX_OCCLUSION);
    } else {
        terrainMaterial->bumpMethod(BumpMethod::NONE);
    }

    terrainMaterial->baseColour(FColour4(DefaultColours::WHITE.rgb * 0.5f, 1.0f));
    terrainMaterial->metallic(0.0f);
    terrainMaterial->roughness(0.8f);
    terrainMaterial->parallaxFactor(terrain->parallaxHeightScale());
    terrainMaterial->toggleTransparency(false);

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

    ResourceDescriptor textureWaterCaustics("Terrain Helper Textures_" + name);
    textureWaterCaustics.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    textureWaterCaustics.assetName(helperTextures);
    textureWaterCaustics.propertyDescriptor(helperTexDescriptor);

    TextureDescriptor normalsTexDescriptor(TextureType::TEXTURE_2D);

    ResourceDescriptor textureNormals("Terrain Normals Textures_" + name);
    textureNormals.assetLocation(terrainLocation);
    textureNormals.assetName(terrainDescriptor->getVariable("normalTex"));
    textureNormals.propertyDescriptor(normalsTexDescriptor);
    textureNormals.flag(true);

    ResourceDescriptor heightMapTexture("Terrain Heightmap_" + name);
    heightMapTexture.assetLocation(terrainLocation);
    heightMapTexture.assetName(terrainDescriptor->getVariable("heightfieldTex"));

    TextureDescriptor heightMapDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_SHORT);
    heightMapDescriptor.autoMipMaps(false);
    heightMapTexture.propertyDescriptor(heightMapDescriptor);
    heightMapTexture.flag(true);

    terrainMaterial->setTexture(TextureUsage::TERRAIN_SPLAT, CreateResource<Texture>(terrain->parentResourceCache(), textureBlendMap), blendMapHash);
    terrainMaterial->setTexture(TextureUsage::TERRAIN_ALBEDO_TILE, CreateResource<Texture>(terrain->parentResourceCache(), textureAlbedoMaps), albedoHash);
    terrainMaterial->setTexture(TextureUsage::TERRAIN_NORMAL_TILE, CreateResource<Texture>(terrain->parentResourceCache(), textureNormalMaps), albedoHash);
    terrainMaterial->setTexture(TextureUsage::TERRAIN_EXTRA_TILE, CreateResource<Texture>(terrain->parentResourceCache(), textureExtraMaps), albedoHash);
    terrainMaterial->setTexture(TextureUsage::TERRAIN_HELPER_TEXTURES, CreateResource<Texture>(terrain->parentResourceCache(), textureWaterCaustics), albedoHash);
    terrainMaterial->setTexture(TextureUsage::TERRAIN_WORLD_NORMALS, CreateResource<Texture>(terrain->parentResourceCache(), textureNormals), blendMapHash);
    terrainMaterial->setTexture(TextureUsage::HEIGHTMAP, CreateResource<Texture>(terrain->parentResourceCache(), heightMapTexture), heightSamplerHash);
    
    terrainMaterial->textureUseForDepth(TextureUsage::TERRAIN_SPLAT, true);
    terrainMaterial->textureUseForDepth(TextureUsage::TERRAIN_NORMAL_TILE, true);
    terrainMaterial->textureUseForDepth(TextureUsage::TERRAIN_HELPER_TEXTURES, true);
    terrainMaterial->textureUseForDepth(TextureUsage::TERRAIN_WORLD_NORMALS, true);
    terrainMaterial->textureUseForDepth(TextureUsage::HEIGHTMAP, true);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "terrainTess.glsl";

    ShaderModuleDescriptor tescModule = {};
    tescModule._moduleType = ShaderType::TESSELLATION_CTRL;
    tescModule._sourceFile = "terrainTess.glsl";

    ShaderModuleDescriptor teseModule = {};
    teseModule._moduleType = ShaderType::TESSELLATION_EVAL;
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

    Texture_ptr albedoTile = terrainMaterial->getTexture(TextureUsage::TERRAIN_ALBEDO_TILE).lock();
    WAIT_FOR_CONDITION(albedoTile->getState() == ResourceState::RES_LOADED);

    const U16 tileMapSize = albedoTile->width();
    bool hasGeometryPass = false;
    for (ShaderModuleDescriptor& shaderModule : shaderDescriptor._modules) {
        if (terrainDescriptor->wireframeDebug() != TerrainDescriptor::WireframeMode::NONE) {
            hasGeometryPass = true;
            shaderModule._defines.emplace_back("TOGGLE_DEBUG", true);
            if (terrainDescriptor->wireframeDebug() == TerrainDescriptor::WireframeMode::EDGES) {
                shaderModule._defines.emplace_back("TOGGLE_WIREFRAME", true);
            } else if (terrainDescriptor->wireframeDebug() == TerrainDescriptor::WireframeMode::NORMALS) {
                shaderModule._defines.emplace_back("TOGGLE_NORMALS", true);
                hasGeometryPass = true;
            }
        }

        shaderModule._defines.emplace_back(Util::StringFormat("PATCHES_PER_TILE_EDGE %d", TessellationParams::PATCHES_PER_TILE_EDGE), true);
        shaderModule._defines.emplace_back(Util::StringFormat("CONTROL_VTX_PER_TILE_EDGE %d", TessellationParams::VTX_PER_TILE_EDGE), true);

        if (context.config().rendering.terrainDetailLevel > 0) {
            if (terrainDescriptor->parallaxMode() != TerrainDescriptor::ParallaxMode::NONE) {
                shaderModule._defines.emplace_back("HAS_PARALLAX", true);
            }
            if (context.config().rendering.terrainDetailLevel > 1) {
                shaderModule._defines.emplace_back("REDUCE_TEXTURE_TILE_ARTIFACT", true);
                if (context.config().rendering.terrainDetailLevel > 2) {
                    shaderModule._defines.emplace_back("REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS", true);
                    if (context.config().rendering.terrainDetailLevel > 3) {
                        shaderModule._defines.emplace_back("HIGH_QUALITY_TILE_ARTIFACT_REDUCTION", true);
                    }
                }
            }
        }

        const vec2<F32> uvDivisor = (terrain->tessParams().WorldScale() * TessellationParams::PATCHES_PER_TILE_EDGE) - TessellationParams::PATCHES_PER_TILE_EDGE;

        shaderModule._defines.emplace_back("COMPUTE_TBN", true);

        shaderModule._defines.emplace_back("OVERRIDE_DATA_IDX", true);
        shaderModule._defines.emplace_back("TEXTURE_TILE_SIZE " + Util::to_string(tileMapSize), true);
        shaderModule._defines.emplace_back("TERRAIN_HEIGHT_OFFSET " + Util::to_string(altitudeRange.x) + "f", true);
        shaderModule._defines.emplace_back("TERRAIN_HEIGHT " + Util::to_string(altitudeRange.y - altitudeRange.x) + "f", true);
        shaderModule._defines.emplace_back("WORLD_SCALE_X " + Util::to_string(terrain->tessParams().WorldScale().width) + "f", true);
        shaderModule._defines.emplace_back("WORLD_SCALE_Z " + Util::to_string(terrain->tessParams().WorldScale().height) + "f", true);
        shaderModule._defines.emplace_back("TERRAIN_WIDTH " + Util::to_string(terrainDescriptor->dimensions().width), true);
        shaderModule._defines.emplace_back("TERRAIN_LENGTH " + Util::to_string(terrainDescriptor->dimensions().height), true);
        shaderModule._defines.emplace_back("UV_DIV_X " + Util::to_string(uvDivisor.width) + "f", true);
        shaderModule._defines.emplace_back("UV_DIV_Z " + Util::to_string(uvDivisor.height) + "f", true);
        shaderModule._defines.emplace_back("NODE_STATIC", true);

        shaderModule._defines.emplace_back(Util::StringFormat("MAX_TEXTURE_LAYERS %d", layerCount), true);
        shaderModule._defines.emplace_back(Util::StringFormat("TEXTURE_SPLAT %d", to_base(TextureUsage::TERRAIN_SPLAT)), true);
        shaderModule._defines.emplace_back(Util::StringFormat("TEXTURE_ALBEDO_TILE %d", to_base(TextureUsage::TERRAIN_ALBEDO_TILE)), true);
        shaderModule._defines.emplace_back(Util::StringFormat("TEXTURE_NORMAL_TILE %d", to_base(TextureUsage::TERRAIN_NORMAL_TILE)), true);
        shaderModule._defines.emplace_back(Util::StringFormat("TEXTURE_EXTRA_TILE %d", to_base(TextureUsage::TERRAIN_EXTRA_TILE)), true);
        shaderModule._defines.emplace_back(Util::StringFormat("TEXTURE_HELPER_TEXTURES %d", to_base(TextureUsage::TERRAIN_HELPER_TEXTURES)), true);
        shaderModule._defines.emplace_back(Util::StringFormat("TEXTURE_WORLD_NORMALS %d", to_base(TextureUsage::TERRAIN_WORLD_NORMALS)), true);

        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._defines.emplace_back(blendAmntStr, true);

            if (!context.config().rendering.shadowMapping.enabled) {
                shaderModule._defines.emplace_back("DISABLE_SHADOW_MAPPING", true);
            }

            shaderModule._defines.emplace_back("SKIP_TEX0", true);
            shaderModule._defines.emplace_back("USE_SHADING_COOK_TORRANCE", true);
            shaderModule._defines.emplace_back(Util::StringFormat("UNDERWATER_TILE_SCALE %d", to_I32(underwaterTileScale)), true);
            shaderModule._defines.emplace_back(Util::StringFormat("TOTAL_LAYER_COUNT %d", totalLayerCount), true);
            shaderModule._defines.emplace_back(layerCountData, false);
            for (const stringImpl& str : indexData) {
                shaderModule._defines.emplace_back(str, false);
            }

        } else if ((hasGeometryPass && shaderModule._moduleType == ShaderType::GEOMETRY) ||
                   (!hasGeometryPass && shaderModule._moduleType == ShaderType::TESSELLATION_EVAL)) {
            shaderModule._defines.emplace_back("HAS_CLIPPING_OUT", true);
        }
    }

    // SHADOW
    ShaderProgramDescriptor shadowDescriptorVSM = {}; ;
    for (const ShaderModuleDescriptor& shaderModule : shaderDescriptor._modules) {
        shadowDescriptorVSM._modules.push_back(shaderModule);
        ShaderModuleDescriptor& tempModule = shadowDescriptorVSM._modules.back();

        if (tempModule._moduleType == ShaderType::FRAGMENT) {
            tempModule._sourceFile = "depthPass";
            tempModule._variant = "Shadow.VSM";
        }
        tempModule._defines.emplace_back("SHADOW_PASS", true);
        tempModule._defines.emplace_back("MAX_TESS_LEVEL 32.f", true);
    }

    ResourceDescriptor terrainShaderShadowVSM("Terrain_ShadowVSM-" + name);
    terrainShaderShadowVSM.propertyDescriptor(shadowDescriptorVSM);

    ShaderProgram_ptr terrainShadowShaderVSM = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderShadowVSM);

    // MAIN PASS
    ShaderProgramDescriptor colourDescriptor = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : colourDescriptor._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "MainPass";
        }
        shaderModule._defines.emplace_back("USE_SSAO", true);
        shaderModule._defines.emplace_back("USE_DEFERRED_NORMALS", true);
    }

    ResourceDescriptor terrainShaderColour("Terrain_Colour-" + name);
    terrainShaderColour.propertyDescriptor(colourDescriptor);

    ShaderProgram_ptr terrainColourShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderColour);

    // PRE PASS
    ShaderProgramDescriptor prePassDescriptor = colourDescriptor;
    for (ShaderModuleDescriptor& shaderModule : prePassDescriptor._modules) {
        shaderModule._defines.emplace_back("PRE_PASS", true);
    }

    ResourceDescriptor terrainShaderPrePass("Terrain_PrePass-" + name);
    terrainShaderPrePass.propertyDescriptor(prePassDescriptor);

    ShaderProgram_ptr terrainPrePassShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderPrePass);

    // PRE PASS LQ
    ShaderProgramDescriptor prePassDescriptorLQ = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : prePassDescriptorLQ._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "";
        }
        shaderModule._defines.emplace_back("PRE_PASS", true);
        shaderModule._defines.emplace_back("LOW_QUALITY", true);
        shaderModule._defines.emplace_back("MAX_TESS_LEVEL 16.f", true);
    }

    ResourceDescriptor terrainShaderPrePassLQ("Terrain_PrePass_LowQuality-" + name);
    terrainShaderPrePassLQ.propertyDescriptor(prePassDescriptorLQ);

    ShaderProgram_ptr terrainPrePassShaderLQ = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderPrePassLQ);

    // MAIN PASS LQ
    ShaderProgramDescriptor lowQualityDescriptor = shaderDescriptor;
    for (ShaderModuleDescriptor& shaderModule : lowQualityDescriptor._modules) {
        if (shaderModule._moduleType == ShaderType::FRAGMENT) {
            shaderModule._variant = "LQPass";
        }

        shaderModule._defines.emplace_back("LOW_QUALITY", true);
        shaderModule._defines.emplace_back("MAX_TESS_LEVEL 16.f", true);
    }

    ResourceDescriptor terrainShaderColourLQ("Terrain_Colour_LowQuality-" + name);
    terrainShaderColourLQ.propertyDescriptor(lowQualityDescriptor);

    ShaderProgram_ptr terrainColourShaderLQ = CreateResource<ShaderProgram>(terrain->parentResourceCache(), terrainShaderColourLQ);

    terrainMaterial->setShaderProgram(terrainPrePassShaderLQ, RenderStage::COUNT,   RenderPassType::PRE_PASS);
    terrainMaterial->setShaderProgram(terrainColourShaderLQ,  RenderStage::COUNT,   RenderPassType::MAIN_PASS);
    terrainMaterial->setShaderProgram(terrainPrePassShader,   RenderStage::DISPLAY, RenderPassType::PRE_PASS);
    terrainMaterial->setShaderProgram(terrainColourShader,    RenderStage::DISPLAY, RenderPassType::MAIN_PASS);
    terrainMaterial->setShaderProgram(terrainShadowShaderVSM, RenderStage::SHADOW,  RenderPassType::COUNT);

    RenderStateBlock terrainRenderState = {};
    terrainRenderState.setTessControlPoints(4);
    terrainRenderState.setCullMode(CullMode::BACK);
    terrainRenderState.setZFunc(ComparisonFunction::EQUAL);

    RenderStateBlock terrainRenderStatePrePass = terrainRenderState;
    terrainRenderStatePrePass.setZFunc(ComparisonFunction::LEQUAL);

    { //Normal rendering
        terrainMaterial->setRenderStateBlock(terrainRenderStatePrePass.getHash(), RenderStage::COUNT, RenderPassType::PRE_PASS);
        terrainMaterial->setRenderStateBlock(terrainRenderState.getHash()       , RenderStage::COUNT, RenderPassType::MAIN_PASS);
    }
    { //Reflected rendering
        RenderStateBlock terrainRenderStateReflection = terrainRenderState;
        terrainRenderStateReflection.setCullMode(CullMode::FRONT);

        RenderStateBlock terrainRenderStateReflectionPrePass = terrainRenderStatePrePass;
        terrainRenderStateReflectionPrePass.setCullMode(CullMode::FRONT);

        terrainMaterial->setRenderStateBlock(terrainRenderStateReflectionPrePass.getHash(), RenderStage::REFLECTION, RenderPassType::PRE_PASS,  to_U8(ReflectorType::PLANAR));
        terrainMaterial->setRenderStateBlock(terrainRenderStateReflection.getHash(),        RenderStage::REFLECTION, RenderPassType::MAIN_PASS, to_U8(ReflectorType::PLANAR));

    }
    { //Shadow rendering
        RenderStateBlock terrainRenderStateShadow = terrainRenderStatePrePass;
        terrainRenderStateShadow.setColourWrites(true, true, false, false);
        terrainRenderStateShadow.setZFunc(ComparisonFunction::LESS);
        //terrainRenderStateShadow.setCullMode(CullMode::FRONT);
        //terrainRenderStateShadow.setZBias(4.0f, 20.0f);

        terrainMaterial->setRenderStateBlock(terrainRenderStateShadow.getHash(), RenderStage::SHADOW, RenderPassType::COUNT);
    }

    terrain->setMaterialTpl(terrainMaterial);

    if (threadedLoading) {
        Start(*CreateTask(context.taskPool(TaskPoolType::HIGH_PRIORITY), [terrain, terrainDescriptor, &context](const Task & parent) {
            loadThreadedResources(terrain, context, terrainDescriptor);
        }));
    } else {
        loadThreadedResources(terrain, context, terrainDescriptor);
    }

    return true;
}

bool TerrainLoader::loadThreadedResources(Terrain_ptr terrain,
                                          PlatformContext& context,
                                          const std::shared_ptr<TerrainDescriptor> terrainDescriptor) {

    stringImpl terrainMapLocation = Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor->getVariable("descriptor");
    stringImpl terrainRawFile(terrainDescriptor->getVariable("heightfield"));

    const vec2<U16>& terrainDimensions = terrainDescriptor->dimensions();
    
    const F32 minAltitude = terrainDescriptor->altitudeRange().x;
    const F32 maxAltitude = terrainDescriptor->altitudeRange().y;
    BoundingBox& terrainBB = Attorney::TerrainLoader::boundingBox(*terrain);
    terrainBB.set(vec3<F32>(-terrainDimensions.x * 0.5f, minAltitude, -terrainDimensions.y * 0.5f),
                  vec3<F32>( terrainDimensions.x * 0.5f, maxAltitude,  terrainDimensions.y * 0.5f));

    const vec3<F32>& bMin = terrainBB.getMin();
    const vec3<F32>& bMax = terrainBB.getMax();

    ByteBuffer terrainCache;
    if (!g_disableLoadFromCache && terrainCache.loadFromFile((Paths::g_cacheLocation + Paths::g_terrainCacheLocation).c_str(), (terrainRawFile + ".cache").c_str())) {
        terrainCache >> terrain->_physicsVerts;
    }

    if (terrain->_physicsVerts.empty()) {

        vectorEASTL<Byte> data(to_size(terrainDimensions.width) * terrainDimensions.height * (sizeof(U16) / sizeof(char)), Byte{0});
        readFile((terrainMapLocation + "/").c_str(), terrainRawFile.c_str(), data, FileType::BINARY);

        constexpr F32 ushortMax = std::numeric_limits<U16>::max() + 1.0f;

        I32 terrainWidth = to_I32(terrainDimensions.x);
        I32 terrainHeight = to_I32(terrainDimensions.y);

        terrain->_physicsVerts.resize(to_size(terrainWidth) * terrainHeight);

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
        terrainCache.dumpToFile((Paths::g_cacheLocation + Paths::g_terrainCacheLocation).c_str(), (terrainRawFile + ".cache").c_str());
    }

    // Do this first in case we have any threaded loads
    VegetationDetails& vegDetails = initializeVegetationDetails(terrain, context, terrainDescriptor);
    // Then compute quadtree and all additional terrain-related structures
    Attorney::TerrainLoader::postBuild(*terrain);
    Vegetation::createAndUploadGPUData(context.gfx(), terrain, vegDetails);

    Console::printfn(Locale::get(_ID("TERRAIN_LOAD_END")), terrain->resourceName().c_str());
    return terrain->load();
}

VegetationDetails& TerrainLoader::initializeVegetationDetails(std::shared_ptr<Terrain> terrain,
                                                              PlatformContext& context,
                                                              const std::shared_ptr<TerrainDescriptor> terrainDescriptor) {
    VegetationDetails& vegDetails = Attorney::TerrainLoader::vegetationDetails(*terrain);

    const U32 terrainWidth = terrainDescriptor->dimensions().width;
    const U32 terrainHeight = terrainDescriptor->dimensions().height;
    const U16 chunkSize = terrainDescriptor->chunkSize();
    const U32 maxChunkCount = to_U32(std::ceil((terrainWidth * terrainHeight) / (chunkSize * chunkSize * 1.0f)));

    Vegetation::precomputeStaticData(context.gfx(), chunkSize, maxChunkCount);

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
 
    stringImpl currentImage = terrainDescriptor->getVariable("grassBillboard1");
    if (!currentImage.empty()) {
        vegDetails.billboardTextureArray += currentImage;
        vegDetails.billboardCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard2");
    if (!currentImage.empty()) {
        vegDetails.billboardTextureArray += "," + currentImage;
        vegDetails.billboardCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard3");
    if (!currentImage.empty()) {
        vegDetails.billboardTextureArray += "," + currentImage;
        vegDetails.billboardCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard4");
    if (!currentImage.empty()) {
        vegDetails.billboardTextureArray += "," + currentImage;
        vegDetails.billboardCount++;
    }

    vegDetails.name = terrain->resourceName() + "_vegetation";
    vegDetails.parentTerrain = terrain;

    stringImpl terrainLocation = Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor->getVariable("descriptor");

    vegDetails.grassMap.reset(new ImageTools::ImageData);
    ImageTools::ImageDataInterface::CreateImageData(terrainLocation + "/" + terrainDescriptor->getVariable("grassMap"),
                                                    0, 0, false,
                                                    *vegDetails.grassMap);

    vegDetails.treeMap.reset(new ImageTools::ImageData);
    ImageTools::ImageDataInterface::CreateImageData(terrainLocation + "/" + terrainDescriptor->getVariable("treeMap"),
                                                    0, 0, false,
                                                    *vegDetails.treeMap);

    return vegDetails;
}

bool TerrainLoader::Save(const char* fileName) { return true; }

bool TerrainLoader::Load(const char* filename) { return true; }

const boost::property_tree::ptree& empty_ptree() {
    static boost::property_tree::ptree t;
    return t;
}
};
