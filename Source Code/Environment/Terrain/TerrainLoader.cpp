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
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Patch3D.h"

namespace Divide {

bool TerrainLoader::loadTerrain(std::shared_ptr<Terrain> terrain,
                                const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                                PlatformContext& context,
                                bool threadedLoading,
                                const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback ) {
    const stringImpl& name = terrainDescriptor->getVariable("terrainName");

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

    SamplerDescriptor heightMapSampler;
    heightMapSampler.setWrapMode(TextureWrap::CLAMP);
    heightMapSampler.setAnisotropy(0);
    heightMapSampler.toggleMipMaps(false);

    const stringImpl& terrainMapLocation = terrainDescriptor->getVariable("textureLocation");

    // Blend map
    ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name);
    textureBlendMap.setEnumValue(to_base(TextureType::TEXTURE_2D_ARRAY));
    textureBlendMap.setResourceLocation(terrainMapLocation);
    textureBlendMap.setPropertyDescriptor(blendMapSampler);
    textureBlendMap.setFlag(true);

    // Albedo map
    ResourceDescriptor textureTileMaps("Terrain Tile Maps_" + name);
    textureTileMaps.setEnumValue(to_base(TextureType::TEXTURE_2D_ARRAY));
    textureTileMaps.setResourceLocation(terrainMapLocation);
    textureTileMaps.setPropertyDescriptor(albedoSampler);
    textureTileMaps.setFlag(true);

    // Normal map
    ResourceDescriptor textureNormalMaps("Terrain Normal Maps_" + name);
    textureNormalMaps.setEnumValue(to_base(TextureType::TEXTURE_2D_ARRAY));
    textureNormalMaps.setResourceLocation(terrainMapLocation);
    textureNormalMaps.setPropertyDescriptor(normalSampler);
    textureNormalMaps.setFlag(true);

    //temp data
    stringImpl layerOffsetStr;
    stringImpl currentEntry;

    //texture data
    stringImpl blendMapArray;
    stringImpl albedoMapArray;
    stringImpl normalMapArray;
    U8 albedoCount = 0;
    U8 normalCount = 0;

    U8 layerCount = terrainDescriptor->getTextureLayerCount();
    TerrainTextureLayer* textureLayer = MemoryManager_NEW TerrainTextureLayer(layerCount);

    for (U8 i = 0; i < layerCount; ++i) {
        layerOffsetStr = to_stringImpl(i);
        
        blendMapArray += ((i != 0) ? (",") : ("")) + terrainDescriptor->getVariable("blendMap" + layerOffsetStr);
        
        currentEntry = terrainDescriptor->getVariable("redAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += ((i != 0) ? (",") : ("")) + currentEntry;
            albedoCount++;

            textureLayer->setDiffuseScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_RED_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("diffuseScaleR" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("greenAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += "," + currentEntry;
            albedoCount++;

            
            textureLayer->setDiffuseScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_GREEN_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("diffuseScaleG" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("blueAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += "," + currentEntry;
            albedoCount++;
            
            textureLayer->setDiffuseScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_BLUE_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("diffuseScaleB" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("alphaAlbedo" + layerOffsetStr);
        if (!currentEntry.empty()) {
            albedoMapArray += "," + currentEntry;
            albedoCount++;
            
            textureLayer->setDiffuseScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL,
                                          i,
                                          terrainDescriptor->getVariablef("diffuseScaleA" + layerOffsetStr));
        }


        currentEntry =  terrainDescriptor->getVariable("redDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += ((i != 0) ? (",") : ("")) + currentEntry;
            normalCount++;

            textureLayer->setDetailScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_RED_CHANNEL,
                                         i,
                                         terrainDescriptor->getVariablef("detailScaleR" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("greenDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += "," + currentEntry;
            normalCount++;

            textureLayer->setDetailScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_GREEN_CHANNEL,
                                         i,
                                         terrainDescriptor->getVariablef("detailScaleG" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("blueDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += "," + currentEntry;
            normalCount++;

            textureLayer->setDetailScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_BLUE_CHANNEL,
                                         i,
                                         terrainDescriptor->getVariablef("detailScaleB" + layerOffsetStr));
        }

        currentEntry = terrainDescriptor->getVariable("alphaDetail" + layerOffsetStr);
        if (!currentEntry.empty()) {
            normalMapArray += "," + currentEntry;
            normalCount++;

            textureLayer->setDetailScale(TerrainTextureLayer::TerrainTextureChannel::TEXTURE_ALPHA_CHANNEL,
                                         i,
                                         terrainDescriptor->getVariablef("detailScaleA" + layerOffsetStr));
        }
    }

    textureBlendMap.setID(layerCount);
    textureBlendMap.setResourceName(blendMapArray);

    textureTileMaps.setID(albedoCount);
    textureTileMaps.setResourceName(albedoMapArray);

    textureNormalMaps.setID(normalCount);
    textureNormalMaps.setResourceName(normalMapArray);

    textureLayer->setBlendMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureBlendMap));
    textureLayer->setTileMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureTileMaps));
    textureLayer->setNormalMaps(CreateResource<Texture>(terrain->parentResourceCache(), textureNormalMaps));
    Attorney::TerrainLoader::setTextureLayer(*terrain, textureLayer);

    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material_ptr terrainMaterial = CreateResource<Material>(terrain->parentResourceCache(), terrainMaterialDescriptor);

    const vec2<U16>& terrainDimensions = terrainDescriptor->getDimensions();
    const vec2<F32>& altitudeRange = terrainDescriptor->getAltitudeRange();

    Console::d_printfn(Locale::get(_ID("TERRAIN_INFO")), terrainDimensions.width, terrainDimensions.height);

    Attorney::TerrainLoader::dimensions(*terrain).set(terrainDimensions.width, terrainDimensions.height);
    Attorney::TerrainLoader::scaleFactor(*terrain).set(terrainDescriptor->getScale());
    Attorney::TerrainLoader::offsetPosition(*terrain).set(terrainDescriptor->getPosition());
    Attorney::TerrainLoader::altitudeRange(*terrain).set(altitudeRange);

    F32 underwaterDiffuseScale = terrainDescriptor->getVariablef("underwaterDiffuseScale");

    terrainMaterial->setDiffuse(
        vec4<F32>(DefaultColours::WHITE().rgb() / 2, 1.0f));
    terrainMaterial->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    terrainMaterial->setShininess(20.0f);
    terrainMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    terrainMaterial->setShaderDefines("COMPUTE_TBN");
    terrainMaterial->setShaderDefines("SKIP_TEXTURES");
    terrainMaterial->setShaderDefines("USE_SHADING_PHONG");
    terrainMaterial->setShaderDefines("MAX_TEXTURE_LAYERS " + to_stringImpl(Attorney::TerrainLoader::textureLayerCount(*terrain)));
    terrainMaterial->setShaderDefines("CURRENT_TEXTURE_COUNT " + to_stringImpl(albedoCount));
    terrainMaterial->setShaderDefines("TERRAIN_WIDTH " + to_stringImpl(terrainDimensions.width));
    terrainMaterial->setShaderDefines("TERRAIN_LENGTH " + to_stringImpl(terrainDimensions.height));
    terrainMaterial->setShaderDefines("TERRAIN_MIN_HEIGHT " + to_stringImpl(altitudeRange.x));
    terrainMaterial->setShaderDefines("TERRAIN_HEIGHT_RANGE " + to_stringImpl(altitudeRange.y - altitudeRange.x));
    terrainMaterial->setShaderDefines("UNDERWATER_DIFFUSE_SCALE " + to_stringImpl(underwaterDiffuseScale));
    terrainMaterial->setShaderProgram("terrainTess." + name, true);
    terrainMaterial->setShaderProgram("terrainTess.Shadow." + name, RenderStage::SHADOW, true);
    terrainMaterial->setShaderProgram("terrainTess.PrePass." + name, RenderPassType::DEPTH_PASS, true);

    ResourceDescriptor textureWaterCaustics("Terrain Water Caustics_" + name);
    textureWaterCaustics.setResourceLocation(terrainMapLocation);
    textureWaterCaustics.setResourceName(terrainDescriptor->getVariable("waterCaustics"));
    textureWaterCaustics.setPropertyDescriptor(albedoSampler);
    textureWaterCaustics.setEnumValue(to_base(TextureType::TEXTURE_2D));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, CreateResource<Texture>(terrain->parentResourceCache(), textureWaterCaustics));

    ResourceDescriptor underwaterAlbedoTexture("Terrain Underwater Albedo_" + name);
    underwaterAlbedoTexture.setResourceLocation(terrainMapLocation);
    underwaterAlbedoTexture.setResourceName(terrainDescriptor->getVariable("underwaterAlbedoTexture"));
    underwaterAlbedoTexture.setPropertyDescriptor(albedoSampler);
    underwaterAlbedoTexture.setEnumValue(to_base(TextureType::TEXTURE_2D));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::UNIT1, CreateResource<Texture>(terrain->parentResourceCache(), underwaterAlbedoTexture));

    ResourceDescriptor underwaterDetailTexture("Terrain Underwater Detail_" + name);
    underwaterDetailTexture.setResourceLocation(terrainMapLocation);
    underwaterDetailTexture.setResourceName(terrainDescriptor->getVariable("underwaterDetailTexture"));
    underwaterDetailTexture.setPropertyDescriptor(albedoSampler);
    underwaterDetailTexture.setEnumValue(to_base(TextureType::TEXTURE_2D));
    terrainMaterial->setTexture(ShaderProgram::TextureUsage::NORMALMAP, CreateResource<Texture>(terrain->parentResourceCache(), underwaterDetailTexture));

    ResourceDescriptor heightMapTexture("Terrain Heightmap_" + name);
    heightMapTexture.setResourceLocation(terrainDescriptor->getVariable("heightmapLocation"));
    heightMapTexture.setResourceName(terrainDescriptor->getVariable("heightmap"));
    heightMapTexture.setPropertyDescriptor(heightMapSampler);
    heightMapTexture.setFlag(true);
    heightMapTexture.setEnumValue(to_base(TextureType::TEXTURE_2D));

    terrainMaterial->setTexture(ShaderProgram::TextureUsage::OPACITY, CreateResource<Texture>(terrain->parentResourceCache(), heightMapTexture));
    terrainMaterial->setShaderLoadThreaded(false);

    terrainMaterial->dumpToFile(false);
    terrain->setMaterialTpl(terrainMaterial);

    // Generate a render state
    RenderStateBlock terrainRenderState;
    terrainRenderState.setCullMode(CullMode::CCW);
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

    Attorney::TerrainLoader::chunkSize(*terrain) = terrainDescriptor->getChunkSize();

    const stringImpl& terrainMapLocation = terrainDescriptor->getVariable("heightmapLocation");
    stringImpl terrainRawFile(terrainDescriptor->getVariable("heightmap"));

    const vec2<U16>& terrainDimensions = Attorney::TerrainLoader::dimensions(*terrain);
    
    F32 minAltitude = terrainDescriptor->getAltitudeRange().x;
    F32 maxAltitude = terrainDescriptor->getAltitudeRange().y;
    BoundingBox& terrainBB = Attorney::TerrainLoader::boundingBox(*terrain);
    terrainBB.set(vec3<F32>(-terrainDimensions.x * 0.5f, minAltitude, -terrainDimensions.y * 0.5f),
                  vec3<F32>( terrainDimensions.x * 0.5f, maxAltitude,  terrainDimensions.y * 0.5f));

    const vec3<F32>& bMin = terrainBB.getMin();
    const vec3<F32>& bMax = terrainBB.getMax();

    stringImpl cacheLocation(terrainMapLocation + terrainRawFile + ".cache");
    ByteBuffer terrainCache;
    if (!terrainCache.loadFromFile(cacheLocation)) {
        F32 altitudeRange = maxAltitude - minAltitude;

        vectorImpl<U16> heightValues;
        if (terrainDescriptor->is16Bit()) {
            assert(terrainDimensions.x != 0 && terrainDimensions.y != 0);
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
                heightValues.push_back((data[i + 1] << 8) | data[i]);
            }

        } else {
            ImageTools::ImageData img;
            //img.flip(true);
            ImageTools::ImageDataInterface::CreateImageData(terrainMapLocation + terrainRawFile, img);
            assert(terrainDimensions == img.dimensions());

            // data will be destroyed when img gets out of scope
            const U8* data = (const U8*)img.data();
            assert(data);
            heightValues.insert(std::end(heightValues), &data[0], &data[img.imageLayers().front()._data.size()]);
        }


        I32 terrainWidth = to_I32(terrainDimensions.x);
        I32 terrainHeight = to_I32(terrainDimensions.y);

        terrain->_physicsVerts.resize(terrainWidth * terrainHeight);

        // scale and translate all height by half to convert from 0-255 (0-65335) to -127 - 128 (-32767 - 32768)

        auto highDetailHeight = [minAltitude, altitudeRange](F32 height) -> F32 {
            constexpr F32 fMax = std::numeric_limits<U16>::max() + 1.0f;
            return minAltitude + altitudeRange * (height / fMax);
        };

        auto lowDetailHeight = [minAltitude, altitudeRange](F32 height) -> F32 {
            constexpr F32 byteMax = std::numeric_limits<U8>::max() + 1.0f;
            return minAltitude + altitudeRange * (height / byteMax);
        };
        
        F32 bXRange = bMax.x - bMin.x;
        F32 bZRange = bMax.z - bMin.z;

        #pragma omp parallel for
        for (I32 j = 0; j < terrainHeight; j++) {
            for (I32 i = 0; i < terrainWidth; i++) {
                U32 idxIMG = TER_COORD<U32>(i < to_I32(terrainDimensions.x) ? i : i - 1,
                                            j < to_I32(terrainDimensions.y) ? j : j - 1,
                                            terrainDimensions.x);

                F32 heightValue = ((heightValues[idxIMG * 3 + 0] +
                                    heightValues[idxIMG * 3 + 1] +
                                    heightValues[idxIMG * 3 + 2]) / 3.0f);

                vec3<F32> vertexData(bMin.x + (to_F32(i)) * bXRange / (terrainWidth - 1),          //X
                                     terrainDescriptor->is16Bit() ? highDetailHeight(heightValue)
                                                                  : lowDetailHeight(heightValue),  //Y
                                     bMin.z + (to_F32(j)) * (bZRange) / (terrainHeight - 1));      //Z

                    
                U32 targetCoord = TER_COORD(i, j, terrainWidth);

                #pragma omp critical
                {
                    terrain->_physicsVerts[targetCoord]._position.set(vertexData);
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

                vU.set(terrain->_physicsVerts[TER_COORD(i + offset, j + 0, terrainWidth)]._position -
                       terrain->_physicsVerts[TER_COORD(i - offset, j + 0, terrainWidth)]._position);
                vV.set(terrain->_physicsVerts[TER_COORD(i + 0, j + offset, terrainWidth)]._position -
                       terrain->_physicsVerts[TER_COORD(i + 0, j - offset, terrainWidth)]._position);

                vUV.cross(vV, vU);
                vUV.normalize();
                vU = -vU;
                vU.normalize();
                #pragma omp critical
                {
                    terrain->_physicsVerts[idx]._normal = Util::PACK_VEC3(vUV);
                    terrain->_physicsVerts[idx]._tangent = Util::PACK_VEC3(vU);
                }
            }
        }
        
        vec3<F32> normal, tangent;
        for (I32 j = 0; j < offset; j++) {
            for (I32 i = 0; i < terrainWidth; i++) {
                idx0 = TER_COORD(i, j, terrainWidth);
                idx1 = TER_COORD(i, offset, terrainWidth);

                normal.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._normal));
                tangent.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._tangent));

                terrain->_physicsVerts[idx0]._normal = Util::PACK_VEC3(normal);
                terrain->_physicsVerts[idx0]._tangent = Util::PACK_VEC3(tangent);

                idx0 = TER_COORD(i, terrainHeight - 1 - j, terrainWidth);
                idx1 = TER_COORD(i, terrainHeight - 1 - offset, terrainWidth);

                normal.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._normal));
                tangent.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._tangent));

                terrain->_physicsVerts[idx0]._normal = Util::PACK_VEC3(normal);
                terrain->_physicsVerts[idx0]._tangent = Util::PACK_VEC3(tangent);
            }
        }

        for (I32 i = 0; i < offset; i++) {
            for (I32 j = 0; j < terrainHeight; j++) {
                idx0 = TER_COORD(i, j, terrainWidth);
                idx1 = TER_COORD(offset, j, terrainWidth);

                normal.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._normal));
                tangent.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._tangent));

                terrain->_physicsVerts[idx0]._normal = Util::PACK_VEC3(normal);
                terrain->_physicsVerts[idx0]._tangent = Util::PACK_VEC3(tangent);

                idx0 = TER_COORD(terrainWidth - 1 - i, j, terrainWidth);
                idx1 = TER_COORD(terrainWidth - 1 - offset, j, terrainWidth);

                normal.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._normal));
                tangent.set(Util::UNPACK_VEC3(terrain->_physicsVerts[idx1]._tangent));

                terrain->_physicsVerts[idx0]._normal = Util::PACK_VEC3(normal);
                terrain->_physicsVerts[idx0]._tangent = Util::PACK_VEC3(tangent);
            }
        }
        terrainCache << terrain->_physicsVerts;
        terrainCache.dumpToFile(cacheLocation);
    } else {
        terrainCache >> terrain->_physicsVerts;
    }

    VertexBuffer* vb = terrain->getGeometryVB();
    Attorney::TerrainTessellatorLoader::initTessellationPatch(vb);
    vb->keepData(false);
    vb->create(true);

    initializeVegetation(terrain, terrainDescriptor);
    Attorney::TerrainLoader::buildQuadtree(*terrain);

    F32 underwaterDiffuseScale = terrainDescriptor->getVariablef("underwaterDiffuseScale");

    ResourceDescriptor planeShaderDesc("terrainPlane.Colour");
    planeShaderDesc.setPropertyList("UNDERWATER_DIFFUSE_SCALE " + to_stringImpl(underwaterDiffuseScale));
    ShaderProgram_ptr planeShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), planeShaderDesc);
    planeShaderDesc.setResourceName("terrainPlane.Depth");
    ShaderProgram_ptr planeDepthShader = CreateResource<ShaderProgram>(terrain->parentResourceCache(), planeShaderDesc);

    Quad3D_ptr plane = CreateResource<Quad3D>(terrain->parentResourceCache(), infinitePlane);
    // our bottom plane is placed at the bottom of our water entity
    F32 height = bMin.y;
    F32 farPlane = 2.0f * terrainBB.getWidth();

    plane->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(-farPlane * 1.5f, height, -farPlane * 1.5f));
    plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(farPlane * 1.5f, height, -farPlane * 1.5f));
    plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(-farPlane * 1.5f, height, farPlane * 1.5f));
    plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(farPlane * 1.5f, height, farPlane * 1.5f));

    Attorney::TerrainLoader::plane(*terrain, plane, planeShader, planeDepthShader);

    ResourceDescriptor renderGeometry("TerrainRenderGeometry");
    renderGeometry.setFlag(true);  // No default material
    renderGeometry.setUserPtr((void*)(&terrainDimensions));

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
    grassSampler.toggleSRGBColourSpace(true);

    ResourceDescriptor textureDetailMaps("Vegetation Billboards");
    textureDetailMaps.setEnumValue(to_base(TextureType::TEXTURE_2D_ARRAY));
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
