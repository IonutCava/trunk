#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainLoader.h"
#include "Headers/TerrainDescriptor.h"
#include "Headers/TerrainTessellator.h"

#include "Core/Headers/PlatformContext.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

namespace Divide {

namespace {
    constexpr bool g_disableLoadFromCache = false;
};

bool TerrainLoader::loadTerrain(Terrain_ptr terrain,
                                const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                                PlatformContext& context,
                                bool threadedLoading,
                                const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback ) {
    const stringImpl& name = terrainDescriptor->getVariable("terrainName");
    const stringImpl& terrainMapLocation = Paths::g_assetsLocation + terrainDescriptor->getVariable("textureLocation");
    Attorney::TerrainLoader::descriptor(*terrain, terrainDescriptor);

    // Blend map
    ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name);
    textureBlendMap.assetLocation(terrainMapLocation);
    textureBlendMap.setFlag(true);

    // Albedo map
    ResourceDescriptor textureTileMaps("Terrain Tile Maps_" + name);
    textureTileMaps.assetLocation(terrainMapLocation);
    textureTileMaps.setFlag(true);

    // Normal map
    ResourceDescriptor textureNormalMaps("Terrain Normal Maps_" + name);
    textureNormalMaps.assetLocation(terrainMapLocation);
    textureNormalMaps.setFlag(true);

    //temp data
    stringImpl layerOffsetStr;
    stringImpl currentEntry;

    //texture data
    stringImpl blendMapArray;
    stringImpl albedoMapArray;
    stringImpl normalMapArray;

    U8 layerCount = terrainDescriptor->getTextureLayerCount();
    vector<U8> albedoCount(layerCount);
    vector<U8> normalCount(layerCount);

    TerrainTextureLayer* textureLayer = MemoryManager_NEW TerrainTextureLayer(layerCount);
    for (U8 i = 0; i < layerCount; ++i) {
        layerOffsetStr = to_stringImpl(i);
        albedoCount[i] = 0;
        normalCount[i] = 0;

        blendMapArray += ((i != 0) ? (",") : ("")) + terrainDescriptor->getVariable("blendMap" + layerOffsetStr);
        
        currentEntry = terrainDescriptor->getVariable("redAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += ((i != 0) ? (",") : ("")) + currentEntry;
            ++albedoCount[i];

            textureLayer->setTileScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_RED_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("redTileScale" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("greenAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += "," + currentEntry;
            ++albedoCount[i];

            
            textureLayer->setTileScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_GREEN_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("greenTileScale" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("blueAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += "," + currentEntry;
            ++albedoCount[i];
            
            textureLayer->setTileScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_BLUE_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("blueTileScale" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("alphaAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += "," + currentEntry;
            ++albedoCount[i];
            
            textureLayer->setTileScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("alphaTileScale" + layerOffsetStr));
        }


        currentEntry =  terrainDescriptor->getVariable("redDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += ((i != 0) ? (",") : ("")) + currentEntry;
            ++normalCount[i];
        }

        currentEntry = terrainDescriptor->getVariable("greenDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += "," + currentEntry;
            ++normalCount[i];
        }

        currentEntry = terrainDescriptor->getVariable("blueDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += "," + currentEntry;
            ++normalCount[i];
        }

        currentEntry = terrainDescriptor->getVariable("alphaDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += "," + currentEntry;
            ++normalCount[i];
        }
    }

    SamplerDescriptor heightMapSampler = {};
    heightMapSampler._wrapU = TextureWrap::CLAMP;
    heightMapSampler._wrapV = TextureWrap::CLAMP;
    heightMapSampler._wrapW = TextureWrap::CLAMP;
    heightMapSampler._minFilter = TextureFilter::LINEAR;
    heightMapSampler._magFilter = TextureFilter::LINEAR;
    heightMapSampler._anisotropyLevel = 0;

    SamplerDescriptor blendMapSampler = {};
    blendMapSampler._wrapU = TextureWrap::CLAMP;
    blendMapSampler._wrapV = TextureWrap::CLAMP;
    blendMapSampler._wrapW = TextureWrap::CLAMP;
    blendMapSampler._minFilter = TextureFilter::LINEAR;
    blendMapSampler._magFilter = TextureFilter::LINEAR;
    blendMapSampler._anisotropyLevel = 0;
    
    SamplerDescriptor albedoSampler = {};
    albedoSampler._wrapU = TextureWrap::REPEAT;
    albedoSampler._wrapV = TextureWrap::REPEAT;
    albedoSampler._wrapW = TextureWrap::REPEAT;
    albedoSampler._minFilter = TextureFilter::LINEAR;
    albedoSampler._magFilter = TextureFilter::LINEAR;
    albedoSampler._anisotropyLevel = 8;

    SamplerDescriptor normalSampler = {};
    normalSampler._wrapU = TextureWrap::REPEAT;
    normalSampler._wrapV = TextureWrap::REPEAT;
    normalSampler._wrapW = TextureWrap::REPEAT;
    normalSampler._minFilter = TextureFilter::LINEAR;
    normalSampler._magFilter = TextureFilter::LINEAR;
    normalSampler._anisotropyLevel = 8;

    TextureDescriptor blendMapDescriptor(TextureType::TEXTURE_2D_ARRAY);
    blendMapDescriptor.setSampler(blendMapSampler);
    blendMapDescriptor._layerCount = layerCount;

    TextureDescriptor albedoDescriptor(TextureType::TEXTURE_2D_ARRAY);
    albedoDescriptor.setSampler(albedoSampler);
    albedoDescriptor._layerCount = std::accumulate(std::cbegin(albedoCount), std::cend(albedoCount), 0u);
    albedoDescriptor._srgb = true;

    TextureDescriptor normalDescriptor(TextureType::TEXTURE_2D_ARRAY);
    normalDescriptor.setSampler(normalSampler);
    normalDescriptor._layerCount = std::accumulate(std::cbegin(normalCount), std::cend(normalCount), 0u);

    textureBlendMap.assetName(blendMapArray);
    textureBlendMap.setPropertyDescriptor(blendMapDescriptor);

    textureTileMaps.assetName(albedoMapArray);
    textureTileMaps.setPropertyDescriptor(albedoDescriptor);
    
    textureNormalMaps.assetName(normalMapArray);
    textureNormalMaps.setPropertyDescriptor(normalDescriptor);

    textureLayer->setBlendMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureBlendMap));
    textureLayer->setTileMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureTileMaps), albedoCount);
    textureLayer->setNormalMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureNormalMaps), normalCount);
    Attorney::TerrainLoader::setTextureLayer(*terrain, textureLayer);

    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material_ptr terrainMaterial = CreateResource<Material>(terrain->parentResourceCache(), terrainMaterialDescriptor);
    terrainMaterial->ignoreXMLData(true);

    const vec2<U16>& terrainDimensions = terrainDescriptor->getDimensions();
    const vec2<F32>& altitudeRange = terrainDescriptor->getAltitudeRange();

    Console::d_printfn(Locale::get(_ID("TERRAIN_INFO")), terrainDimensions.width, terrainDimensions.height);

    F32 underwaterTileScale = terrainDescriptor->getVariablef("underwaterTileScale");
    terrainMaterial->setDiffuse(FColour(DefaultColours::WHITE.rgb() * 0.5f, 1.0f));
    terrainMaterial->setSpecular(FColour(0.1f, 0.1f, 0.1f, 1.0f));
    terrainMaterial->setShininess(20.0f);
    terrainMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);

    stringImpl layerCountData = "const uint CURRENT_LAYER_COUNT[" + to_stringImpl(layerCount) + "] = {";
    for (U8 i = 0; i < layerCount; ++i) {
        layerCountData += to_stringImpl(albedoCount[i]);
        if (i < layerCount - 1) {
            layerCountData += ",";
        }
    }
    layerCountData += "};";

    if (!Terrain::USE_TERRAIN_UBO) {
        terrainMaterial->addShaderDefine("USE_SSBO_DATA_BUFFER");
    }

    //terrainMaterial->addShaderDefine("TOGGLE_WIREFRAME");
    terrainMaterial->addShaderDefine("COMPUTE_TBN");
    terrainMaterial->addShaderDefine("SKIP_TEXTURES");
    terrainMaterial->addShaderDefine("USE_SHADING_PHONG");
    terrainMaterial->addShaderDefine("MAX_TEXTURE_LAYERS " + to_stringImpl(Attorney::TerrainLoader::textureLayerCount(*terrain)));
    
    terrainMaterial->addShaderDefine(layerCountData, false);
    terrainMaterial->addShaderDefine("MAX_RENDER_NODES " + to_stringImpl(Terrain::MAX_RENDER_NODES));
    terrainMaterial->addShaderDefine("TERRAIN_WIDTH " + to_stringImpl(terrainDimensions.width));
    terrainMaterial->addShaderDefine("TERRAIN_LENGTH " + to_stringImpl(terrainDimensions.height));
    terrainMaterial->addShaderDefine("TERRAIN_MIN_HEIGHT " + to_stringImpl(altitudeRange.x));
    terrainMaterial->addShaderDefine("TERRAIN_HEIGHT_RANGE " + to_stringImpl(altitudeRange.y - altitudeRange.x));
    terrainMaterial->addShaderDefine("UNDERWATER_TILE_SCALE " + to_stringImpl(underwaterTileScale));

    TextureDescriptor miscTexDescriptor(TextureType::TEXTURE_2D);
    miscTexDescriptor.setSampler(albedoSampler);

    ResourceDescriptor textureWaterCaustics("Terrain Water Caustics_" + name);
    textureWaterCaustics.assetLocation(terrainMapLocation);
    textureWaterCaustics.assetName(terrainDescriptor->getVariable("waterCaustics"));
    textureWaterCaustics.setPropertyDescriptor(miscTexDescriptor);
    
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, CreateResource<Texture>(terrain->parentResourceCache(), textureWaterCaustics));

    ResourceDescriptor underwaterAlbedoTexture("Terrain Underwater Albedo_" + name);
    underwaterAlbedoTexture.assetLocation(terrainMapLocation);
    underwaterAlbedoTexture.assetName(terrainDescriptor->getVariable("underwaterAlbedoTexture"));
    underwaterAlbedoTexture.setPropertyDescriptor(miscTexDescriptor);
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::UNIT1, CreateResource<Texture>(terrain->parentResourceCache(), underwaterAlbedoTexture));

    ResourceDescriptor underwaterDetailTexture("Terrain Underwater Detail_" + name);
    underwaterDetailTexture.assetLocation(terrainMapLocation);
    underwaterDetailTexture.assetName(terrainDescriptor->getVariable("underwaterDetailTexture"));
    underwaterDetailTexture.setPropertyDescriptor(miscTexDescriptor);
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::NORMALMAP, CreateResource<Texture>(terrain->parentResourceCache(), underwaterDetailTexture));

    ResourceDescriptor heightMapTexture("Terrain Heightmap_" + name);
    heightMapTexture.assetLocation(Paths::g_assetsLocation + terrainDescriptor->getVariable("heightmapLocation"));
    heightMapTexture.assetName(terrainDescriptor->getVariable("heightmap"));


    TextureDescriptor heightMapDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, terrainDescriptor->is16Bit() ? GFXDataFormat::UNSIGNED_SHORT : GFXDataFormat::UNSIGNED_BYTE);
    heightMapDescriptor.setSampler(heightMapSampler);
    heightMapTexture.setPropertyDescriptor(heightMapDescriptor);

    heightMapTexture.setFlag(true);

    terrainMaterial->setTexture(ShaderProgram::TextureUsage::OPACITY, CreateResource<Texture>(terrain->parentResourceCache(), heightMapTexture));

    terrainMaterial->addShaderDefine(RenderStage::REFLECTION, "LOW_QUALITY", true);
    terrainMaterial->addShaderDefine(RenderStage::REFRACTION, "LOW_QUALITY", true);

    terrainMaterial->setShaderProgram("terrainTess.Colour" + name, RenderStage::DISPLAY, true);
    terrainMaterial->setShaderProgram("terrainTess.Colour.LowQuality" + name, RenderStage::REFLECTION, true);
    terrainMaterial->setShaderProgram("terrainTess.Colour.LowQuality" + name, RenderStage::REFRACTION, true);
    terrainMaterial->setShaderProgram("terrainTess.PrePass." + name, RenderPassType::DEPTH_PASS, true);
    terrainMaterial->setShaderProgram("terrainTess.Shadow." + name, RenderStage::SHADOW, true);

    terrainMaterial->dumpToFile(false);
    terrain->setMaterialTpl(terrainMaterial);

    // Generate a render state
    RenderStateBlock terrainRenderState;
    terrainRenderState.setCullMode(CullMode::CW);
    // Generate a render state for drawing reflections
    RenderStateBlock terrainRenderStateReflection;
    terrainRenderStateReflection.setCullMode(CullMode::CCW);
    // Generate a shadow render state
    RenderStateBlock terrainRenderStateDepth;
    terrainRenderStateDepth.setCullMode(CullMode::CCW);
    // terrainDescDepth.setZBias(1.0f, 1.0f);
    terrainRenderStateDepth.setColourWrites(true, true, false, false);

    terrainMaterial->setRenderStateBlock(terrainRenderState.getHash());
    terrainMaterial->setRenderStateBlock(terrainRenderStateReflection.getHash(), RenderStage::REFLECTION);
    terrainMaterial->setRenderStateBlock(terrainRenderStateDepth.getHash(), RenderStage::SHADOW);

    CreateTask(context, [terrain, terrainDescriptor, onLoadCallback](const Task& parent) {
        loadThreadedResources(terrain, std::move(terrainDescriptor), std::move(onLoadCallback));
    }).startTask(threadedLoading ? TaskPriority::DONT_CARE : TaskPriority::REALTIME);

    return true;
}

bool TerrainLoader::loadThreadedResources(Terrain_ptr terrain,
                                          const std::shared_ptr<TerrainDescriptor> terrainDescriptor,
                                          DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) {

    const stringImpl& terrainMapLocation = Paths::g_assetsLocation + terrainDescriptor->getVariable("heightmapLocation");
    stringImpl terrainRawFile(terrainDescriptor->getVariable("heightmap"));

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
        ImageTools::ImageData img;
        img.set16Bit(terrainDescriptor->is16Bit());
        //img.flip(true);
        ImageTools::ImageDataInterface::CreateImageData(terrainMapLocation + "/" + terrainRawFile, img);

        assert(terrainDimensions == img.dimensions());
        U8 componentCount = img.bpp() / (img.is16Bit() ? 16 : 8);
        assert(componentCount > 0);

        constexpr F32 ushortMax = std::numeric_limits<U16>::max() + 1.0f;
        constexpr F32 ubyteMax = std::numeric_limits<U8>::max() + 1.0f;

        I32 terrainWidth = to_I32(terrainDimensions.x);
        I32 terrainHeight = to_I32(terrainDimensions.y);

        bool is16Bit = terrainDescriptor->is16Bit();
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
                if (is16Bit) {
                    U16* data = static_cast<U16*>(img.data16(0));
                    for (U8 j = 0; j < componentCount; ++j) {
                        yOffset += data[idxIMG * componentCount + j];
                    }
                    yOffset /= componentCount;
                    yOffset = (altitudeRange * (yOffset / ushortMax)) + minAltitude;
                } else {
                    Byte* data = static_cast<Byte*>(img.data(0));
                    for (U8 j = 0; j < componentCount; ++j) {
                        yOffset += data[idxIMG * componentCount + j];
                    }
                    yOffset /= componentCount;
                    yOffset = (altitudeRange * (yOffset / ubyteMax)) + minAltitude;
                }

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
    return terrain->load(onLoadCallback);
}

void TerrainLoader::initializeVegetation(std::shared_ptr<Terrain> terrain,
                                         const std::shared_ptr<TerrainDescriptor> terrainDescriptor) {
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
    grassSampler._wrapU = TextureWrap::CLAMP;
    grassSampler._wrapV = TextureWrap::CLAMP;
    grassSampler._wrapW = TextureWrap::CLAMP;
    grassSampler._anisotropyLevel = 8;

    TextureDescriptor grassTexDescriptor(TextureType::TEXTURE_2D_ARRAY);
    grassTexDescriptor._layerCount = textureCount;
    grassTexDescriptor.setSampler(grassSampler);
    grassTexDescriptor._srgb = true;
    grassTexDescriptor.automaticMipMapGeneration(true);

    ResourceDescriptor textureDetailMaps("Vegetation Billboards");
    textureDetailMaps.assetLocation(Paths::g_assetsLocation + terrainDescriptor->getVariable("grassMapLocation"));
    textureDetailMaps.assetName(textureName);
    textureDetailMaps.setPropertyDescriptor(grassTexDescriptor);
    
    Texture_ptr grassBillboardArray = CreateResource<Texture>(terrain->parentResourceCache(), textureDetailMaps);

    VegetationDetails& vegDetails = Attorney::TerrainLoader::vegetationDetails(*terrain);
    vegDetails.billboardCount = textureCount;
    vegDetails.name = terrain->resourceName() + "_grass";
    vegDetails.grassDensity = terrainDescriptor->getGrassDensity();
    vegDetails.grassScale = terrainDescriptor->getGrassScale();
    vegDetails.parentTerrain = terrain;

    ResourceDescriptor vegetationMaterial("grassMaterial");
    Material_ptr vegMaterial = CreateResource<Material>(terrain->parentResourceCache(), vegetationMaterial);

    vegMaterial->setDiffuse(DefaultColours::WHITE);
    vegMaterial->setSpecular(FColour(0.1f, 0.1f, 0.1f, 1.0f));
    vegMaterial->setShininess(5.0f);
    vegMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    vegMaterial->addShaderDefine("SKIP_TEXTURES");
    vegMaterial->addShaderDefine(RenderPassType::OIT_PASS, "OIT_PASS", true);
    vegMaterial->setShaderProgram("grass.Colour", true);
    vegMaterial->setShaderProgram("grass.PrePass", RenderPassType::DEPTH_PASS, true);
    vegMaterial->setShaderProgram("grass.Shadow", RenderStage::SHADOW, true);

    vegMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, grassBillboardArray);
    vegMaterial->dumpToFile(false);
    vegDetails.vegetationMaterialPtr = vegMaterial;

    vegDetails.map.reset(new ImageTools::ImageData);
    ImageTools::ImageDataInterface::CreateImageData(Paths::g_assetsLocation + 
                                                    terrainDescriptor->getVariable("grassMapLocation") +
                                                    "/" +
                                                    terrainDescriptor->getVariable("grassMap"),
                                                    *vegDetails.map);
}

bool TerrainLoader::Save(const char* fileName) { return true; }

bool TerrainLoader::Load(const char* filename) { return true; }
};
