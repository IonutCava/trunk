#include "Headers/Terrain.h"
#include "Headers/TerrainLoader.h"
#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

bool TerrainLoader::loadTerrain(std::shared_ptr<Terrain> terrain,
                                const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                                PlatformContext& context,
                                bool threadedLoading,
                                const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback ) {
    const stringImpl& name = terrainDescriptor->getVariable("terrainName");

    Attorney::TerrainLoader::setUnderwaterDiffuseScale(
        *terrain, terrainDescriptor->getVariablef("underwaterDiffuseScale"));

    SamplerDescriptor blendMapSampler;
    blendMapSampler.setWrapMode(TextureWrap::CLAMP);
    blendMapSampler.setAnisotropy(0);
    blendMapSampler.toggleMipMaps(false);

    SamplerDescriptor albedoSampler;
    albedoSampler.setWrapMode(TextureWrap::REPEAT);
    albedoSampler.setAnisotropy(8);
    albedoSampler.toggleMipMaps(true);
    albedoSampler.toggleSRGBColourSpace(true);

    SamplerDescriptor normalSampler;
    normalSampler.setWrapMode(TextureWrap::REPEAT);
    normalSampler.setAnisotropy(8);
    normalSampler.toggleMipMaps(true);
    normalSampler.toggleSRGBColourSpace(false);

    TerrainTextureLayer* textureLayer = nullptr;
    stringImpl layerOffsetStr;
    stringImpl currentTexture;
    stringImpl arrayLocation;
    U32 textureCount = 0;
    U32 textureCountAlbedo = 0;
    U32 textureCountDetail = 0;
    const stringImpl& terrainMapLocation = terrainDescriptor->getVariable("textureLocation");

    for (U32 i = 0; i < terrainDescriptor->getTextureLayerCount(); ++i) {
        textureCountAlbedo = 0;
        textureCountDetail = 0;

        layerOffsetStr = to_stringImpl(i);
        textureLayer = MemoryManager_NEW TerrainTextureLayer();

        ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name + "_layer_" + layerOffsetStr);
        textureBlendMap.setResourceLocation(terrainMapLocation);
        textureBlendMap.setResourceName(terrainDescriptor->getVariable("blendMap" + layerOffsetStr));
        textureBlendMap.setPropertyDescriptor(blendMapSampler);
        textureBlendMap.setEnumValue(to_const_U32(TextureType::TEXTURE_2D));
        textureLayer->setBlendMap(CreateResource<Texture>(terrain->parentResourceCache(), textureBlendMap));

        arrayLocation.clear();
        currentTexture = terrainDescriptor->getVariable("redAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation = currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_RED_CHANNEL,
                terrainDescriptor->getVariablef("diffuseScaleR" +
                                                layerOffsetStr));
        }
        currentTexture =
            terrainDescriptor->getVariable("greenAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_GREEN_CHANNEL,
                terrainDescriptor->getVariablef("diffuseScaleG" +
                                                layerOffsetStr));
        }
        currentTexture =
            terrainDescriptor->getVariable("blueAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_BLUE_CHANNEL,
                terrainDescriptor->getVariablef("diffuseScaleB" +
                                                layerOffsetStr));
        }
        currentTexture =
            terrainDescriptor->getVariable("alphaAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL,
                terrainDescriptor->getVariablef("diffuseScaleA" +
                                                layerOffsetStr));
        }

        ResourceDescriptor textureTileMaps("Terrain Tile Maps_" + name +
                                           "_layer_" + layerOffsetStr);
        textureTileMaps.setEnumValue(to_const_U32(TextureType::TEXTURE_2D_ARRAY));
        textureTileMaps.setID(textureCountAlbedo);
        textureTileMaps.setResourceLocation(terrainMapLocation);
        textureTileMaps.setResourceName(arrayLocation);
        textureTileMaps.setPropertyDescriptor(albedoSampler);
        textureLayer->setTileMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureTileMaps));

        arrayLocation.clear();
        currentTexture =
            terrainDescriptor->getVariable("redDetail" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_RED_CHANNEL,
                terrainDescriptor->getVariablef("detailScaleR" +
                                                layerOffsetStr));
        }
        currentTexture =
            terrainDescriptor->getVariable("greenDetail" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_GREEN_CHANNEL,
                terrainDescriptor->getVariablef("detailScaleG" +
                                                layerOffsetStr));
        }
        currentTexture =
            terrainDescriptor->getVariable("blueDetail" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_BLUE_CHANNEL,
                terrainDescriptor->getVariablef("detailScaleB" +
                                                layerOffsetStr));
        }
        currentTexture =
            terrainDescriptor->getVariable("alphaDetail" + layerOffsetStr);
        if (!currentTexture.empty()) {
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(
                TerrainTextureLayer::TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL,
                terrainDescriptor->getVariablef("detailScaleA" +
                                                layerOffsetStr));
        }

        ResourceDescriptor textureNormalMaps("Terrain Normal Maps_" + name +
                                             "_layer_" + layerOffsetStr);
        textureNormalMaps.setEnumValue(to_const_U32(TextureType::TEXTURE_2D_ARRAY));
        textureNormalMaps.setID(textureCountDetail);
        textureNormalMaps.setResourceLocation(terrainMapLocation);
        textureNormalMaps.setResourceName(arrayLocation);
        textureNormalMaps.setPropertyDescriptor(normalSampler);
        textureLayer->setNormalMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureNormalMaps));

        Attorney::TerrainLoader::addTextureLayer(*terrain, textureLayer);
    }

    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material_ptr terrainMaterial =
        CreateResource<Material>(terrain->parentResourceCache(), terrainMaterialDescriptor);

    terrainMaterial->setDiffuse(
        vec4<F32>(DefaultColours::WHITE().rgb() / 2, 1.0f));
    terrainMaterial->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    terrainMaterial->setShininess(20.0f);
    terrainMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    terrainMaterial->setShaderDefines("COMPUTE_TBN");
    terrainMaterial->setShaderDefines("SKIP_TEXTURES");
    terrainMaterial->setShaderDefines("USE_SHADING_PHONG");
    terrainMaterial->setShaderDefines("MAX_TEXTURE_LAYERS " + to_stringImpl(Attorney::TerrainLoader::textureLayerCount(*terrain)));
    terrainMaterial->setShaderDefines("CURRENT_TEXTURE_COUNT " + to_stringImpl(textureCount));

    terrainMaterial->setShaderProgram("terrain", true);
    terrainMaterial->setShaderProgram("depthPass.Shadow.Terrain", RenderStage::SHADOW, true);
    terrainMaterial->setShaderProgram("depthPass.PrePass.Terrain", RenderPassType::DEPTH_PASS, true);

    ResourceDescriptor textureWaterCaustics("Terrain Water Caustics_" + name);
    textureWaterCaustics.setResourceLocation(terrainMapLocation);
    textureWaterCaustics.setResourceName(terrainDescriptor->getVariable("waterCaustics"));
    textureWaterCaustics.setPropertyDescriptor(albedoSampler);
    textureWaterCaustics.setEnumValue(to_const_U32(TextureType::TEXTURE_2D));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, CreateResource<Texture>(terrain->parentResourceCache(), textureWaterCaustics));

    ResourceDescriptor underwaterAlbedoTexture("Terrain Underwater Albedo_" + name);
    underwaterAlbedoTexture.setResourceLocation(terrainMapLocation);
    underwaterAlbedoTexture.setResourceName(terrainDescriptor->getVariable("underwaterAlbedoTexture"));
    underwaterAlbedoTexture.setPropertyDescriptor(albedoSampler);
    underwaterAlbedoTexture.setEnumValue(to_const_U32(TextureType::TEXTURE_2D));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::UNIT1, CreateResource<Texture>(terrain->parentResourceCache(), underwaterAlbedoTexture));

    ResourceDescriptor underwaterDetailTexture("Terrain Underwater Detail_" + name);
    underwaterDetailTexture.setResourceLocation(terrainMapLocation);
    underwaterDetailTexture.setResourceName(terrainDescriptor->getVariable("underwaterDetailTexture"));
    underwaterDetailTexture.setPropertyDescriptor(albedoSampler);
    underwaterDetailTexture.setEnumValue(to_const_U32(TextureType::TEXTURE_2D));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::NORMALMAP, CreateResource<Texture>(terrain->parentResourceCache(), underwaterDetailTexture));

    terrainMaterial->setShaderLoadThreaded(false);
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
    // terrainDescDepth.setZBias(1.0f, 2.0f);
    terrainRenderStateDepth.setColourWrites(true, true, false, false);

    terrainMaterial->setRenderStateBlock(terrainRenderState.getHash());
    terrainMaterial->setRenderStateBlock(terrainRenderStateReflection.getHash(), RenderStage::REFLECTION);
    terrainMaterial->setRenderStateBlock(terrainRenderStateDepth.getHash(), RenderStage::SHADOW);

    return context.gfx().loadInContext(threadedLoading ? CurrentContext::GFX_LOADING_CTX : CurrentContext::GFX_RENDERING_CTX,
                                       [terrain, terrainDescriptor, onLoadCallback](const Task& parent) {
                                            loadThreadedResources(terrain, std::move(terrainDescriptor), std::move(onLoadCallback));
                                       });
}

bool TerrainLoader::loadThreadedResources(std::shared_ptr<Terrain> terrain,
                                          const std::shared_ptr<TerrainDescriptor> terrainDescriptor,
                                          DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) {

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.setFlag(true);  // No default material

    vec2<F32>& terrainScaleFactor =
        Attorney::TerrainLoader::scaleFactor(*terrain);

    Attorney::TerrainLoader::chunkSize(*terrain) = terrainDescriptor->getChunkSize();
    terrainScaleFactor.set(terrainDescriptor->getScale());

    const vec2<U16>& terrainDimensions = terrainDescriptor->getDimensions();
    
    U16 heightmapWidth = terrainDimensions.width;
    U16 heightmapHeight = terrainDimensions.height;
    const stringImpl& terrainMapLocation = terrainDescriptor->getVariable("heightmapLocation");

    stringImpl terrainRawFile(terrainDescriptor->getVariable("heightmap"));

    vec2<U16>& dimensions = Attorney::TerrainLoader::dimensions(*terrain);
    Attorney::TerrainLoader::dimensions(*terrain).set(heightmapWidth, heightmapHeight);

    if (dimensions.x % 2 == 0) {
        dimensions.x++;
    }
    if (dimensions.y % 2 == 0) {
        dimensions.y++;
    }
    Console::d_printfn(Locale::get(_ID("TERRAIN_INFO")), dimensions.x, dimensions.y);

    I32 terrainWidth = (I32)dimensions.x;
    I32 terrainHeight = (I32)dimensions.y;
    F32 minAltitude = terrainDescriptor->getAltitudeRange().x;
    F32 maxAltitude = terrainDescriptor->getAltitudeRange().y;

    BoundingBox& terrainBB = Attorney::TerrainLoader::boundingBox(*terrain);
    terrainBB.set(vec3<F32>(-terrainWidth * 0.5f, minAltitude, -terrainHeight * 0.5f),
                  vec3<F32>(terrainWidth * 0.5f, maxAltitude, terrainHeight * 0.5f));

    terrainBB.translate(terrainDescriptor->getPosition());
    terrainBB.multiply(vec3<F32>(terrainScaleFactor.x, terrainScaleFactor.y,
                                 terrainScaleFactor.x));

    const vec3<F32>& bMin = terrainBB.getMin();
    const vec3<F32>& bMax = terrainBB.getMax();
    F32 yOffset = terrainDescriptor->getPosition().y;

    VertexBuffer* groundVB = terrain->getGeometryVB();

    stringImpl cacheLocation(terrainMapLocation + terrainRawFile + ".cache");

    STUBBED("ToDo: Move image data into the ByteBuffer as well to avoid reading height data from images each time we load the terrain! -Ionut");

    ByteBuffer terrainCache;
    if (!terrainCache.loadFromFile(cacheLocation) || !groundVB->deserialize(terrainCache)) {
        F32 altitudeRange = maxAltitude - minAltitude;
        F32 yScaleFactor = terrainScaleFactor.y;

        vectorImpl<U16> heightValues;
        if (terrainDescriptor->is16Bit()) {
            assert(heightmapWidth != 0 && heightmapHeight != 0);
            // only raw files for 16 bit support
            assert(hasExtension(terrainRawFile, "raw"));
            // Read File Data

            vectorImpl<Byte> data;
            readFile(terrainMapLocation + terrainRawFile, data, FileType::BINARY);
            if (data.empty()) {
                return false;
            }

            U32 positionCount = to_U32(data.size());
            heightValues.reserve(positionCount / 2);
            for (U32 i = 0; i < positionCount + 1; i += 2) {
                heightValues.push_back((data[i + 1] << 8) |
                    data[i]);
            }

        } else {
            ImageTools::ImageData img;
            ImageTools::ImageDataInterface::CreateImageData(terrainMapLocation + terrainRawFile, img);
            assert(terrainDimensions == img.dimensions());
            // data will be destroyed when img gets out of scope
            const U8* data = (const U8*)img.data();
            assert(data);
            heightValues.insert(std::end(heightValues), &data[0], &data[img.imageLayers().front()._data.size()]);
        }

        groundVB->resizeVertexCount(terrainWidth * terrainHeight);
        // scale and translate all height by half to convert from 0-255 (0-65335) to -127 - 128 (-32767 - 32768)
        if (terrainDescriptor->is16Bit()) {
            constexpr F32 fMax = to_const_F32(std::numeric_limits<U16>::max() + 1);
            
            #pragma omp parallel for
            for (I32 j = 0; j < terrainHeight; j++) {
                for (I32 i = 0; i < terrainWidth; i++) {
                    U32 idxHM = TER_COORD(i, j, terrainWidth);

                    U32 idxIMG = TER_COORD<U32>(i < to_I32(heightmapWidth) ? i : i - 1,
                                                j < to_I32(heightmapHeight) ? j : j - 1,
                                                heightmapWidth);

                    vec3<F32> vertexData(bMin.x + (to_F32(i)) * (bMax.x - bMin.x) / (terrainWidth - 1),                           // X
                                        ((minAltitude + altitudeRange * (heightValues[idxIMG] / fMax)) * yScaleFactor) + yOffset, // Y
                                        bMin.z + (to_F32(j)) * (bMax.z - bMin.z) / (terrainHeight - 1));                          // Z
                    #pragma omp critical
                    {
                        groundVB->modifyPositionValue(idxHM, vertexData);
                    }
                }
            }
        } else {
            constexpr F32 byteMax = to_const_F32(std::numeric_limits<U8>::max() + 1);

            #pragma omp parallel for
            for (I32 j = 0; j < terrainHeight; j++) {
                for (I32 i = 0; i < terrainWidth; i++) {
                    U32 idxHM = TER_COORD(i, j, terrainWidth);

                    U32 idxIMG = TER_COORD<U32>(i < to_I32(heightmapWidth) ? i : i - 1,
                                                j < to_I32(heightmapHeight) ? j : j - 1,
                                                heightmapWidth);

                    F32 h = ((heightValues[idxIMG * 3 + 0] +
                              heightValues[idxIMG * 3 + 1] +
                              heightValues[idxIMG * 3 + 2]) / 3.0f) / byteMax;

                    vec3<F32> vertexData(bMin.x + (to_F32(i)) * (bMax.x - bMin.x) / (terrainWidth - 1),   //X
                                         ((minAltitude + altitudeRange * h) * yScaleFactor) + yOffset,    //Y
                                         bMin.z + (to_F32(j)) * (bMax.z - bMin.z) / (terrainHeight - 1)); //Z

                    
                    #pragma omp critical
                    {
                        groundVB->modifyPositionValue(idxHM, vertexData);
                    }
                }
            }
        }

        heightValues.clear();

        I32 offset = 2;
        U32 idx = 0, idx0 = 0, idx1 = 0;
    
        #pragma omp parallel for
        for (I32 j = offset; j < terrainHeight - offset; j++) {
            for (I32 i = offset; i < terrainWidth - offset; i++) {
                vec3<F32> vU, vV, vUV;

                idx = TER_COORD(i, j, terrainWidth);

                vU.set(groundVB->getPosition(TER_COORD(i + offset, j + 0, terrainWidth)) -
                       groundVB->getPosition(TER_COORD(i - offset, j + 0, terrainWidth)));
                vV.set(groundVB->getPosition(TER_COORD(i + 0, j + offset, terrainWidth)) -
                       groundVB->getPosition(TER_COORD(i + 0, j - offset, terrainWidth)));
                vUV.cross(vV, vU);
                vUV.normalize();
                vU = -vU;
                vU.normalize();
                #pragma omp critical
                {
                    groundVB->modifyNormalValue(idx, vUV);
                    groundVB->modifyTangentValue(idx, vU);
                }
            }
        }

        vec3<F32> normal, tangent;
        for (I32 j = 0; j < offset; j++) {
            for (I32 i = 0; i < terrainWidth; i++) {
                idx0 = TER_COORD(i, j, terrainWidth);
                idx1 = TER_COORD(i, offset, terrainWidth);

                normal.set(groundVB->getNormal(idx1));
                tangent.set(groundVB->getTangent(idx1));

                groundVB->modifyNormalValue(idx0, normal);
                groundVB->modifyTangentValue(idx0, tangent);

                idx0 = TER_COORD(i, terrainHeight - 1 - j, terrainWidth);
                idx1 = TER_COORD(i, terrainHeight - 1 - offset, terrainWidth);

                normal.set(groundVB->getNormal(idx1));
                tangent.set(groundVB->getTangent(idx1));

                groundVB->modifyNormalValue(idx0, normal);
                groundVB->modifyTangentValue(idx0, tangent);
            }
        }

        for (I32 i = 0; i < offset; i++) {
            for (I32 j = 0; j < terrainHeight; j++) {
                idx0 = TER_COORD(i, j, terrainWidth);
                idx1 = TER_COORD(offset, j, terrainWidth);

                normal.set(groundVB->getNormal(idx1));
                tangent.set(groundVB->getTangent(idx1));

                groundVB->modifyNormalValue(idx0, normal);
                groundVB->modifyTangentValue(idx0, tangent);

                idx0 = TER_COORD(terrainWidth - 1 - i, j, terrainWidth);
                idx1 = TER_COORD(terrainWidth - 1 - offset, j, terrainWidth);

                normal.set(groundVB->getNormal(idx1));
                tangent.set(groundVB->getTangent(idx1));

                groundVB->modifyNormalValue(idx0, normal);
                groundVB->modifyTangentValue(idx0, tangent);
            }
        }
        if (groundVB->serialize(terrainCache)) {
            terrainCache.dumpToFile(cacheLocation);
        }
    }

    groundVB->keepData(true);
    groundVB->create();
    initializeVegetation(terrain, terrainDescriptor);
    Attorney::TerrainLoader::buildQuadtree(*terrain);

    Quad3D_ptr plane = CreateResource<Quad3D>(terrain->parentResourceCache(), infinitePlane);
    // our bottom plane is placed at the bottom of our water entity
    F32 height = bMin.y + yOffset;
    F32 farPlane = 2.0f * terrainBB.getWidth();

    plane->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(-farPlane * 1.5f, height, -farPlane * 1.5f));
    plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(farPlane * 1.5f, height, -farPlane * 1.5f));
    plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(-farPlane * 1.5f, height, farPlane * 1.5f));
    plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(farPlane * 1.5f, height, farPlane * 1.5f));

    Attorney::TerrainLoader::plane(*terrain, plane);

    Console::printfn(Locale::get(_ID("TERRAIN_LOAD_END")), terrain->getName().c_str());
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

    SamplerDescriptor grassSampler;
    grassSampler.setAnisotropy(0);
    grassSampler.setWrapMode(TextureWrap::CLAMP);
    ResourceDescriptor textureDetailMaps("Vegetation Billboards");
    textureDetailMaps.setEnumValue(to_const_U32(TextureType::TEXTURE_2D_ARRAY));
    textureDetailMaps.setID(textureCount);
    textureDetailMaps.setResourceLocation(terrainDescriptor->getVariable("grassMapLocation"));
    textureDetailMaps.setResourceName(textureName);
    textureDetailMaps.setPropertyDescriptor(grassSampler);
    Texture_ptr grassBillboardArray = CreateResource<Texture>(terrain->parentResourceCache(), textureDetailMaps);

    VegetationDetails& vegDetails = Attorney::TerrainLoader::vegetationDetails(*terrain);
    vegDetails.billboardCount = textureCount;
    vegDetails.name = terrain->getName() + "_grass";
    vegDetails.grassDensity = terrainDescriptor->getGrassDensity();
    vegDetails.grassScale = terrainDescriptor->getGrassScale();
    vegDetails.treeDensity = terrainDescriptor->getTreeDensity();
    vegDetails.treeScale = terrainDescriptor->getTreeScale();
    vegDetails.grassBillboards = grassBillboardArray;
    vegDetails.grassShaderName = "grass";
    vegDetails.parentTerrain = terrain;

    vegDetails.map.reset(new ImageTools::ImageData);
    ImageTools::ImageDataInterface::CreateImageData(
        terrainDescriptor->getVariable("grassMapLocation") + 
        terrainDescriptor->getVariable("grassMap"),
        *vegDetails.map);
}

bool TerrainLoader::Save(const char* fileName) { return true; }

bool TerrainLoader::Load(const char* filename) { return true; }
};
