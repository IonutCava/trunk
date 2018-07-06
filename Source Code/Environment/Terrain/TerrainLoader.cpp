#include "Headers/Terrain.h"
#include "Headers/TerrainLoader.h"
#include "Headers/TerrainDescriptor.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

TerrainLoader::TerrainLoader() : Singleton()
{
    _textureSampler = New SamplerDescriptor();
    _textureSampler->setWrapMode(TEXTURE_REPEAT);
    _textureSampler->setAnisotropy(8);
    _textureSampler->toggleMipMaps(true);
}

TerrainLoader::~TerrainLoader()
{
    SAFE_DELETE(_textureSampler);
}

bool TerrainLoader::loadTerrain(Terrain* terrain, TerrainDescriptor* terrainDescriptor){
    const std::string& name = terrainDescriptor->getVariable("terrainName");
  
    Texture* tempTexture = nullptr;

    ResourceDescriptor textureWaterCaustics("Terrain Water Caustics_" + name);
    textureWaterCaustics.setResourceLocation(terrainDescriptor->getVariable("waterCaustics"));
    textureWaterCaustics.setPropertyDescriptor(*_textureSampler);
    tempTexture = CreateResource<Texture>(textureWaterCaustics);
    terrain->setCausticsTex(tempTexture);

    ResourceDescriptor underwaterAlbedoTexture("Terrain Underwater Albedo_" + name);
    underwaterAlbedoTexture.setResourceLocation(terrainDescriptor->getVariable("underwaterAlbedoTexture"));
    underwaterAlbedoTexture.setPropertyDescriptor(*_textureSampler);
    tempTexture = CreateResource<Texture>(underwaterAlbedoTexture);
    terrain->setUnderwaterAlbedoTex(tempTexture);

    ResourceDescriptor underwaterDetailTexture("Terrain Underwater Detail_" + name);
    underwaterDetailTexture.setResourceLocation(terrainDescriptor->getVariable("underwaterDetailTexture"));
    underwaterDetailTexture.setPropertyDescriptor(*_textureSampler);
    tempTexture = CreateResource<Texture>(underwaterDetailTexture);
    terrain->setUnderwaterDetailTex(tempTexture);
    
    terrain->setUnderwaterDiffuseScale(terrainDescriptor->getVariablef("underwaterDiffuseScale"));

    SamplerDescriptor blendMapSampler;
    blendMapSampler.setWrapMode(TEXTURE_CLAMP);
    blendMapSampler.setAnisotropy(0);
    blendMapSampler.toggleMipMaps(false);

    TerrainTextureLayer* textureLayer = nullptr;
    std::string layerOffsetStr;
    std::string currentTexture;
    std::string arrayLocation;
    U32 textureCount = 0;
    U32 textureCountAlbedo = 0;
    U32 textureCountDetail = 0;
    for (U32 i = 0; i < terrainDescriptor->getTextureLayerCount(); ++i){
        textureCountAlbedo = 0;
        textureCountDetail = 0;

        layerOffsetStr = Util::toString(i);
        textureLayer = New TerrainTextureLayer();
        
        ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name + "_layer_" + layerOffsetStr);
        textureBlendMap.setResourceLocation(terrainDescriptor->getVariable("blendMap" + layerOffsetStr));
        textureBlendMap.setPropertyDescriptor(blendMapSampler);
        tempTexture = CreateResource<Texture>(textureBlendMap);
        textureLayer->setTexture(TerrainTextureLayer::TEXTURE_BLEND_MAP, tempTexture);

        arrayLocation.clear();
        currentTexture = terrainDescriptor->getVariable("redAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation = currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_RED_CHANNEL, terrainDescriptor->getVariablef("diffuseScaleR" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("greenAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_GREEN_CHANNEL, terrainDescriptor->getVariablef("diffuseScaleG" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("blueAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_BLUE_CHANNEL, terrainDescriptor->getVariablef("diffuseScaleB" + layerOffsetStr));
        }
      
        currentTexture = terrainDescriptor->getVariable("alphaAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_ALPHA_CHANNEL, terrainDescriptor->getVariablef("diffuseScaleA" + layerOffsetStr));
        }

        ResourceDescriptor textureAlbedoMaps("Terrain Albedo Maps_" + name + "_layer_" + layerOffsetStr);
        textureAlbedoMaps.setEnumValue(TEXTURE_2D_ARRAY);
        textureAlbedoMaps.setId(textureCountAlbedo);
        textureAlbedoMaps.setResourceLocation(arrayLocation);
        textureAlbedoMaps.setPropertyDescriptor(*_textureSampler);
        tempTexture = CreateResource<Texture>(textureAlbedoMaps);
        textureLayer->setTexture(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, tempTexture);

        arrayLocation.clear();
        currentTexture = terrainDescriptor->getVariable("redDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCountDetail++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_RED_CHANNEL, terrainDescriptor->getVariablef("detailScaleR" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("greenDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCountDetail++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_GREEN_CHANNEL, terrainDescriptor->getVariablef("detailScaleG" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("blueDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCountDetail++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_BLUE_CHANNEL, terrainDescriptor->getVariablef("detailScaleB" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("alphaDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += " " + currentTexture;
            textureCountDetail++;
            textureLayer->setTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_ALPHA_CHANNEL, terrainDescriptor->getVariablef("detailScaleA" + layerOffsetStr));
        }

        ResourceDescriptor textureDetailMaps("Terrain Detail Maps_" + name + "_layer_" + layerOffsetStr);
        textureDetailMaps.setEnumValue(TEXTURE_2D_ARRAY);
        textureDetailMaps.setId(textureCountDetail);
        textureDetailMaps.setResourceLocation(arrayLocation);
        textureDetailMaps.setPropertyDescriptor(*_textureSampler);
        tempTexture = CreateResource<Texture>(textureDetailMaps);
        textureLayer->setTexture(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, tempTexture);

        terrain->_terrainTextures.push_back(textureLayer);
    }   
   
    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material* terrainMaterial = CreateResource<Material>(terrainMaterialDescriptor);

    terrainMaterial->setDiffuse(DefaultColors::WHITE());
    terrainMaterial->setAmbient(DefaultColors::WHITE() / 3);
    terrainMaterial->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    terrainMaterial->setShininess(20.0f);
    terrainMaterial->addShaderDefines("COMPUTE_TBN");
    terrainMaterial->addShaderDefines("MAX_TEXTURE_LAYERS " + Util::toString(terrain->_terrainTextures.size()));
    terrainMaterial->addShaderDefines("CURRENT_TEXTURE_COUNT " + Util::toString(textureCount));
    terrainMaterial->setShaderProgram("terrain", FINAL_STAGE, true);
    terrainMaterial->setShaderProgram("depthPass.Shadow", SHADOW_STAGE, true);
    terrainMaterial->setShaderProgram("depthPass.PrePass", Z_PRE_PASS_STAGE, true);
    terrainMaterial->setShaderLoadThreaded(false);
    terrainMaterial->dumpToFile(false);
    terrain->setMaterial(terrainMaterial);

    //Generate a render state
    RenderStateBlockDescriptor terrainDesc;
    terrainDesc.setCullMode(CULL_MODE_CW);
    RenderStateBlockDescriptor terrainDescRef;
    terrainDescRef.setCullMode(CULL_MODE_CCW);
    //Generate a shadow render state
    RenderStateBlockDescriptor terrainDescDepth;
    terrainDescDepth.setCullMode(CULL_MODE_CCW);
    //terrainDescDepth.setZBias(1.0f, 2.0f);
    terrainDescDepth.setColorWrites(true, true, false, false);

    terrain->_terrainRenderState           = GFX_DEVICE.getOrCreateStateBlock(terrainDesc);
    terrain->_terrainPrePassRenderState    = GFX_DEVICE.getOrCreateStateBlock(terrainDesc);
    terrain->_terrainReflectionRenderState = GFX_DEVICE.getOrCreateStateBlock(terrainDescRef);
    terrain->_terrainDepthRenderState      = GFX_DEVICE.getOrCreateStateBlock(terrainDescDepth);

    return loadThreadedResources(terrain, terrainDescriptor);
}

bool TerrainLoader::loadThreadedResources(Terrain* terrain, TerrainDescriptor* terrainDescriptor){
    VertexBuffer* groundVB = terrain->_groundVB;

    terrain->_chunkSize = terrainDescriptor->getChunkSize();
    terrain->_terrainScaleFactor.y = terrainDescriptor->getScale().y;
    terrain->_terrainScaleFactor.x = terrainDescriptor->getScale().x;
    
    const vec2<F32>& terrainScaleFactor = terrain->_terrainScaleFactor;
    const vec2<U16>& terrainDimensions = terrainDescriptor->getDimensions();
    const vectorImpl<vec3<F32> >& normalData  = groundVB->getNormal();
    const vectorImpl<vec3<F32> >& tangentData = groundVB->getTangent();

    U16 heightmapWidth = terrainDimensions.width;
    U16 heightmapHeight = terrainDimensions.height;
    vectorImpl<U16> heightValues;
    if (terrainDescriptor->is16Bit()){
        assert(heightmapWidth != 0 && heightmapHeight != 0);
        std::string terrainRawFile(terrainDescriptor->getVariable("heightmap"));
        assert(terrainRawFile.substr(terrainRawFile.length() - 4, terrainRawFile.length()).compare(".raw") == 0); // only raw files for 16 bit support
        // Read File Data
        FILE* terrainFile = nullptr;
        fopen_s(&terrainFile, terrainRawFile.c_str(), "rb");
        assert(terrainFile);
        U32 lCurPos = ftell(terrainFile);
        fseek(terrainFile, 0, SEEK_END);
        U32 positionCount = ftell(terrainFile);
        fseek(terrainFile, lCurPos, SEEK_SET);
        heightValues.reserve(positionCount / 2);
        U8* dataTemp = New U8[positionCount];
        rewind(terrainFile);
        assert(dataTemp);
        fread(dataTemp, 1, positionCount, terrainFile);
        fclose(terrainFile);
        for (U32 i = 0; i < positionCount + 1; i += 2){
            heightValues.push_back(((U8)dataTemp[i + 1] << 8) | (U8)dataTemp[i]);
        }
        SAFE_DELETE_ARRAY(dataTemp);
    }else{
        ImageTools::ImageData img;
        img.create(terrainDescriptor->getVariable("heightmap"));
        assert(terrainDimensions == img.dimensions());
        //data will be destroyed when img gets out of scope
        const U8* data = img.data();
        assert(data);
        heightValues.reserve(heightmapWidth * heightmapWidth);
        for (size_t i = 0; i < img.imageSize(); ++i){
            heightValues.push_back(data[i]);
        }
    }


    terrain->_terrainWidth = heightmapWidth;
    terrain->_terrainHeight = heightmapHeight;

    if (terrain->_terrainWidth % 2 == 0)  terrain->_terrainWidth++;
    if (terrain->_terrainHeight % 2 == 0) terrain->_terrainHeight++;
    D_PRINT_FN(Locale::get("TERRAIN_INFO"), terrain->_terrainWidth, terrain->_terrainHeight);

    I32 terrainWidth  = (I32)terrain->_terrainWidth;
    I32 terrainHeight = (I32)terrain->_terrainHeight;
    F32 minAltitude = terrainDescriptor->getAltitudeRange().x;
    F32 maxAltitude = terrainDescriptor->getAltitudeRange().y;
    F32 altitudeRange = maxAltitude - minAltitude;

    groundVB->resizePositionCount(terrainWidth *terrainHeight);
    groundVB->resizeNormalCount(terrainWidth * terrainHeight);
    groundVB->resizeTangentCount(terrainWidth * terrainHeight);

    BoundingBox& terrainBB = terrain->_boundingBox;

    terrainBB.set(vec3<F32>(-terrainWidth * 0.5f, minAltitude, -terrainHeight * 0.5f),
                  vec3<F32>( terrainWidth * 0.5f, maxAltitude,  terrainHeight * 0.5f));

    terrainBB.Translate(terrainDescriptor->getPosition());
    terrainBB.Multiply(vec3<F32>(terrainScaleFactor.x, terrainScaleFactor.y, terrainScaleFactor.x));
    F32 yOffset = terrainDescriptor->getPosition().y;
    const vec3<F32>& bMin = terrainBB.getMin();
    const vec3<F32>& bMax = terrainBB.getMax();
    F32 yScaleFactor = terrain->_terrainScaleFactor.y;
    // scale and translate all height by half to convert from 0-255 (0-65335) to -127 - 128 (-32767 - 32768)
    if (terrainDescriptor->is16Bit()){
        #pragma omp parallel for
        for (I32 j = 0; j < terrainHeight; j++) {
            for (I32 i = 0; i < terrainWidth; i++) {
                U32 idxHM = TER_COORD(i, j, terrainWidth);

                vec3<F32> vertexData;
                vertexData.x = bMin.x + ((F32)i) * (bMax.x - bMin.x) / (terrainWidth - 1);
                vertexData.z = bMin.z + ((F32)j) * (bMax.z - bMin.z) / (terrainHeight - 1);

                U32 idxIMG = TER_COORD<U32>(i < (I32)heightmapWidth ? i : i - 1, j < (I32)heightmapHeight ? j : j - 1, heightmapWidth);

                vertexData.y = minAltitude + altitudeRange * (F32)heightValues[idxIMG] / 65536.0f;
                vertexData.y *= yScaleFactor;
                vertexData.y += yOffset;
                #pragma omp critical
                {
                      groundVB->modifyPositionValue(idxHM, vertexData);
                }
            }
        }
    }else{
        #pragma omp parallel for
        for (I32 j = 0; j < terrainHeight; j++) {
            for (I32 i = 0; i < terrainWidth; i++) {
                U32 idxHM = TER_COORD(i, j, terrainWidth);
                vec3<F32> vertexData;

                vertexData.x = bMin.x + ((F32)i) * (bMax.x - bMin.x) / (terrainWidth - 1);
                vertexData.z = bMin.z + ((F32)j) * (bMax.z - bMin.z) / (terrainHeight - 1);

                U32 idxIMG = TER_COORD<U32>(i < (I32)heightmapWidth ? i : i - 1, j < (I32)heightmapHeight ? j : j - 1, heightmapWidth);

                F32 h = (F32)(heightValues[idxIMG * 3 + 0] + heightValues[idxIMG * 3 + 1] + heightValues[idxIMG * 3 + 2]) / 3.0f;

                vertexData.y = minAltitude + altitudeRange * h / 255.0f;
                vertexData.y *= yScaleFactor;
                vertexData.y += yOffset;
                #pragma omp critical
                {
                    groundVB->modifyPositionValue(idxHM , vertexData);
                }
            }
        }
    }

    heightValues.clear();

    const vectorImpl<vec3<F32> >& groundPos = groundVB->getPosition();

    I32 offset = 2;
    vec3<F32> vU, vV, vUV;
    U32 idx = 0, idx0 = 0, idx1 = 0;
    for (I32 j = offset; j < terrainHeight - offset; j++) {
        for (I32 i = offset; i < terrainWidth - offset; i++) {
            idx = TER_COORD(i, j, terrainWidth);

            vU.set(groundPos[TER_COORD(i + offset, j + 0, terrainWidth)] - groundPos[TER_COORD(i - offset, j + 0, terrainWidth)]);
            vV.set(groundPos[TER_COORD(i + 0, j + offset, terrainWidth)] - groundPos[TER_COORD(i + 0, j - offset, terrainWidth)]);
            vUV.cross(vV,vU); 
            vUV.normalize();
            groundVB->modifyNormalValue(idx,vUV);
            vU = -vU; 
            vU.normalize();
            groundVB->modifyTangentValue(idx,vU);
        }
    }

    for(I32 j = 0; j < offset; j++) {
        for (I32 i = 0; i < terrainWidth; i++) {
            idx0 = TER_COORD(i, j,      terrainWidth);
            idx1 = TER_COORD(i, offset, terrainWidth);

            groundVB->modifyNormalValue(idx0, normalData[idx1]);
            groundVB->modifyTangentValue(idx0,tangentData[idx1]);

            idx0 = TER_COORD(i, terrainHeight - 1 - j,      terrainWidth);
            idx1 = TER_COORD(i, terrainHeight - 1 - offset, terrainWidth);

            groundVB->modifyNormalValue(idx0, normalData[idx1]);
            groundVB->modifyTangentValue(idx0,tangentData[idx1]);
        }
    }

    for(I32 i = 0; i < offset; i++) {
        for (I32 j = 0; j < terrainHeight; j++) {
            idx0 = TER_COORD(i,      j, terrainWidth);
            idx1 = TER_COORD(offset, j, terrainWidth);

            groundVB->modifyNormalValue(idx0, normalData[idx1]);
            groundVB->modifyTangentValue(idx0,normalData[idx1]);

            idx0 = TER_COORD(terrainWidth - 1 - i,      j, terrainWidth);
            idx1 = TER_COORD(terrainWidth - 1 - offset, j, terrainWidth);

            groundVB->modifyNormalValue(idx0, normalData[idx1]);
            groundVB->modifyTangentValue(idx0,normalData[idx1]);
        }
    }

    initializeVegetation(terrain, terrainDescriptor);
    PRINT_FN(Locale::get("TERRAIN_LOAD_END"), terrain->getName().c_str());
    return true;
}

void TerrainLoader::initializeVegetation(Terrain* terrain, TerrainDescriptor* terrainDescriptor) {
    U8 textureCount = 0;
    std::string textureLocation;

    std::string currentImage = terrainDescriptor->getVariable("grassBillboard1");
    if (!currentImage.empty()){
        textureLocation += currentImage;
        textureCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard2");
    if (!currentImage.empty()){
        textureLocation += " " + currentImage;
        textureCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard3");
    if (!currentImage.empty()){
        textureLocation += " " + currentImage;
        textureCount++;
    }

    currentImage = terrainDescriptor->getVariable("grassBillboard4");
    if (!currentImage.empty()){
        textureLocation += " " + currentImage;
        textureCount++;
    }

    if (textureCount == 0)
        return;

    SamplerDescriptor grassSampler;
    grassSampler.setAnisotropy(0);
    grassSampler.setWrapMode(TextureWrap::TEXTURE_CLAMP);
    ResourceDescriptor textureDetailMaps("Vegetation Billboards");
    textureDetailMaps.setEnumValue(TEXTURE_2D_ARRAY);
    textureDetailMaps.setId(textureCount);
    textureDetailMaps.setResourceLocation(textureLocation);
    textureDetailMaps.setPropertyDescriptor(grassSampler);
    Texture* grassBillboardArray = CreateResource<Texture>(textureDetailMaps);
    terrain->_vegetation = nullptr;
    terrain->_vegetation = New Vegetation(terrainDescriptor->getName() + "_grass", textureCount,
                                          terrainDescriptor->getGrassDensity(), terrainDescriptor->getGrassScale(),
                                          terrainDescriptor->getTreeDensity(),  terrainDescriptor->getTreeScale(),
                                          terrainDescriptor->getVariable("grassMap"), grassBillboardArray, "grass");
}

bool TerrainLoader::Save(const char* fileName){
    return true;
}

bool TerrainLoader::Load(const char* filename){
    return true;
}