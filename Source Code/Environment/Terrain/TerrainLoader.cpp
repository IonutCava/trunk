#include "Headers/Terrain.h"
#include "Headers/TerrainLoader.h"
#include "Headers/TerrainDescriptor.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

TerrainLoader::TerrainLoader() : Singleton()
{
    _albedoSampler = MemoryManager_NEW SamplerDescriptor();
    _albedoSampler->setWrapMode(TEXTURE_REPEAT);
    _albedoSampler->setAnisotropy(8);
    _albedoSampler->toggleMipMaps(true);
    _albedoSampler->toggleSRGBColorSpace(true);

    _normalSampler = MemoryManager_NEW SamplerDescriptor();
    _normalSampler->setWrapMode(TEXTURE_REPEAT);
    _normalSampler->setAnisotropy(8);
    _normalSampler->toggleMipMaps(true);
}

TerrainLoader::~TerrainLoader()
{
    MemoryManager::DELETE( _albedoSampler );
    MemoryManager::DELETE( _normalSampler );
}

bool TerrainLoader::loadTerrain(Terrain* terrain, TerrainDescriptor* terrainDescriptor){
    const stringImpl& name = terrainDescriptor->getVariable("terrainName");
  
    terrain->setUnderwaterDiffuseScale(terrainDescriptor->getVariablef("underwaterDiffuseScale"));

    SamplerDescriptor blendMapSampler;
    blendMapSampler.setWrapMode(TEXTURE_CLAMP);
    blendMapSampler.setAnisotropy(0);
    blendMapSampler.toggleMipMaps(false);

    TerrainTextureLayer* textureLayer = nullptr;
    stringImpl layerOffsetStr;
    stringImpl currentTexture;
    stringImpl arrayLocation;
    U32 textureCount = 0;
    U32 textureCountAlbedo = 0;
    U32 textureCountDetail = 0;

    for (U32 i = 0; i < terrainDescriptor->getTextureLayerCount(); ++i){
        textureCountAlbedo = 0;
        textureCountDetail = 0;

        layerOffsetStr = Util::toString(i);
        textureLayer = MemoryManager_NEW TerrainTextureLayer();
        
        ResourceDescriptor textureBlendMap("Terrain Blend Map_" + name + "_layer_" + layerOffsetStr);
        textureBlendMap.setResourceLocation(terrainDescriptor->getVariable("blendMap" + layerOffsetStr));
        textureBlendMap.setPropertyDescriptor(blendMapSampler);
        textureLayer->setBlendMap(CreateResource<Texture>(textureBlendMap));

        arrayLocation.clear();
        currentTexture = terrainDescriptor->getVariable("redAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation = currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(TerrainTextureLayer::TEXTURE_RED_CHANNEL, 
                                          terrainDescriptor->getVariablef("diffuseScaleR" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("greenAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(TerrainTextureLayer::TEXTURE_GREEN_CHANNEL, 
                                          terrainDescriptor->getVariablef("diffuseScaleG" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("blueAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(TerrainTextureLayer::TEXTURE_BLUE_CHANNEL, 
                                          terrainDescriptor->getVariablef("diffuseScaleB" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("alphaAlbedo" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCount++;
            textureCountAlbedo++;
            textureLayer->setDiffuseScale(TerrainTextureLayer::TEXTURE_ALPHA_CHANNEL, 
                                          terrainDescriptor->getVariablef("diffuseScaleA" + layerOffsetStr));
        }

        ResourceDescriptor textureTileMaps("Terrain Tile Maps_" + name + "_layer_" + layerOffsetStr);
        textureTileMaps.setEnumValue(TEXTURE_2D_ARRAY);
        textureTileMaps.setId(textureCountAlbedo);
        textureTileMaps.setResourceLocation(arrayLocation);
        textureTileMaps.setPropertyDescriptor(*_albedoSampler);
        textureLayer->setTileMaps(CreateResource<Texture>(textureTileMaps));

        arrayLocation.clear();
        currentTexture = terrainDescriptor->getVariable("redDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(TerrainTextureLayer::TEXTURE_RED_CHANNEL, 
                                         terrainDescriptor->getVariablef("detailScaleR" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("greenDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(TerrainTextureLayer::TEXTURE_GREEN_CHANNEL, 
                                         terrainDescriptor->getVariablef("detailScaleG" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("blueDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(TerrainTextureLayer::TEXTURE_BLUE_CHANNEL, 
                                         terrainDescriptor->getVariablef("detailScaleB" + layerOffsetStr));
        }
        currentTexture = terrainDescriptor->getVariable("alphaDetail" + layerOffsetStr);
        if (!currentTexture.empty()){
            arrayLocation += "," + currentTexture;
            textureCountDetail++;
            textureLayer->setDetailScale(TerrainTextureLayer::TEXTURE_ALPHA_CHANNEL, 
                                         terrainDescriptor->getVariablef("detailScaleA" + layerOffsetStr));
        }

        ResourceDescriptor textureNormalMaps("Terrain Normal Maps_" + name + "_layer_" + layerOffsetStr);
        textureNormalMaps.setEnumValue(TEXTURE_2D_ARRAY);
        textureNormalMaps.setId(textureCountDetail);
        textureNormalMaps.setResourceLocation(arrayLocation);
        textureNormalMaps.setPropertyDescriptor(*_normalSampler);
        textureLayer->setNormalMaps(CreateResource<Texture>(textureTileMaps));

        terrain->_terrainTextures.push_back(textureLayer);
    }   
   
    ResourceDescriptor terrainMaterialDescriptor("terrainMaterial_" + name);
    Material* terrainMaterial = CreateResource<Material>(terrainMaterialDescriptor);

    terrainMaterial->setDiffuse(vec4<F32>(DefaultColors::WHITE().rgb() / 2, 1.0f));
    terrainMaterial->setAmbient(vec4<F32>(DefaultColors::WHITE().rgb() / 3, 1.0f));
    terrainMaterial->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    terrainMaterial->setShininess(20.0f);
    terrainMaterial->setShadingMode(Material::SHADING_BLINN_PHONG);
    terrainMaterial->addShaderDefines("COMPUTE_TBN");
    terrainMaterial->addShaderDefines("SKIP_TEXTURES");
    terrainMaterial->addShaderDefines("MAX_TEXTURE_LAYERS " + Util::toString(terrain->_terrainTextures.size()));
    terrainMaterial->addShaderDefines("CURRENT_TEXTURE_COUNT " + Util::toString(textureCount));
    terrainMaterial->setShaderProgram("terrain", FINAL_STAGE, true);
    terrainMaterial->setShaderProgram("depthPass.Shadow.Terrain", SHADOW_STAGE, true);
    terrainMaterial->setShaderProgram("depthPass.PrePass.Terrain", Z_PRE_PASS_STAGE, true);

    ResourceDescriptor textureWaterCaustics("Terrain Water Caustics_" + name);
    textureWaterCaustics.setResourceLocation(terrainDescriptor->getVariable("waterCaustics"));
    textureWaterCaustics.setPropertyDescriptor(*_albedoSampler);
    terrainMaterial->setTexture(ShaderProgram::TEXTURE_UNIT0, CreateResource<Texture>(textureWaterCaustics));

    ResourceDescriptor underwaterAlbedoTexture("Terrain Underwater Albedo_" + name);
    underwaterAlbedoTexture.setResourceLocation(terrainDescriptor->getVariable("underwaterAlbedoTexture"));
    underwaterAlbedoTexture.setPropertyDescriptor(*_albedoSampler);
    terrainMaterial->setTexture(ShaderProgram::TEXTURE_UNIT1, CreateResource<Texture>(underwaterAlbedoTexture));

    ResourceDescriptor underwaterDetailTexture("Terrain Underwater Detail_" + name);
    underwaterDetailTexture.setResourceLocation(terrainDescriptor->getVariable("underwaterDetailTexture"));
    underwaterDetailTexture.setPropertyDescriptor(*_normalSampler);
    terrainMaterial->setTexture(ShaderProgram::TEXTURE_NORMALMAP, CreateResource<Texture>(underwaterDetailTexture));

    terrainMaterial->setShaderLoadThreaded(false);
    terrainMaterial->dumpToFile(false);
    terrain->setMaterialTpl(terrainMaterial);

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

    terrain->_terrainRenderStateHash           = GFX_DEVICE.getOrCreateStateBlock(terrainDesc);
    terrain->_terrainReflectionRenderStateHash = GFX_DEVICE.getOrCreateStateBlock(terrainDescRef);
    terrain->_terrainDepthRenderStateHash      = GFX_DEVICE.getOrCreateStateBlock(terrainDescDepth);

    return loadThreadedResources(terrain, terrainDescriptor);
}

bool TerrainLoader::loadThreadedResources(Terrain* terrain, TerrainDescriptor* terrainDescriptor){
    ResourceDescriptor infinitePlane( "infinitePlane" );
    infinitePlane.setFlag( true ); //No default material
    terrain->_plane = CreateResource<Quad3D>( infinitePlane );
    // our bottom plane is placed at the bottom of our water entity
    F32 height = GET_ACTIVE_SCENE()->state().getWaterLevel() - GET_ACTIVE_SCENE()->state().getWaterDepth();
    terrain->_farPlane = 2.0f * GET_ACTIVE_SCENE()->state().getRenderState().getCameraConst().getZPlanes().y;
    terrain->_plane->setCorner( Quad3D::TOP_LEFT, vec3<F32>( -terrain->_farPlane * 1.5f, height, -terrain->_farPlane * 1.5f ) );
    terrain->_plane->setCorner( Quad3D::TOP_RIGHT, vec3<F32>( terrain->_farPlane * 1.5f, height, -terrain->_farPlane * 1.5f ) );
    terrain->_plane->setCorner( Quad3D::BOTTOM_LEFT, vec3<F32>( -terrain->_farPlane * 1.5f, height, terrain->_farPlane * 1.5f ) );
    terrain->_plane->setCorner( Quad3D::BOTTOM_RIGHT, vec3<F32>( terrain->_farPlane * 1.5f, height, terrain->_farPlane * 1.5f ) );

    VertexBuffer* groundVB = terrain->getGeometryVB();

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
    if ( terrainDescriptor->is16Bit() ) {
        assert( heightmapWidth != 0 && heightmapHeight != 0 );
        stringImpl terrainRawFile( terrainDescriptor->getVariable( "heightmap" ) );
        // only raw files for 16 bit support
        assert( terrainRawFile.substr( terrainRawFile.length() - 4, terrainRawFile.length() ).compare( ".raw" ) == 0 ); 
        // Read File Data
        FILE* terrainFile = nullptr;
        fopen_s( &terrainFile, terrainRawFile.c_str(), "rb" );
        assert( terrainFile );
        U32 lCurPos = ftell( terrainFile );
        fseek( terrainFile, 0, SEEK_END );
        U32 positionCount = ftell( terrainFile );
        fseek( terrainFile, lCurPos, SEEK_SET );
        heightValues.reserve( positionCount / 2 );
        U8* dataTemp = MemoryManager_NEW U8[positionCount];
        rewind( terrainFile );
        assert( dataTemp );
        fread( dataTemp, 1, positionCount, terrainFile );
        fclose( terrainFile );
        for ( U32 i = 0; i < positionCount + 1; i += 2 ) {
            heightValues.push_back( ( (U8)dataTemp[i + 1] << 8 ) | (U8)dataTemp[i] );
        }
        MemoryManager::DELETE_ARRAY( dataTemp );
    } else {
        ImageTools::ImageData img;
        img.create( terrainDescriptor->getVariable( "heightmap" ) );
        assert( terrainDimensions == img.dimensions() );
        //data will be destroyed when img gets out of scope
        const U8* data = img.data();
        assert( data );
        heightValues.reserve( heightmapWidth * heightmapWidth );
        for ( size_t i = 0; i < img.imageSize(); ++i ) {
            heightValues.push_back( data[i] );
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

                U32 idxIMG = TER_COORD<U32>(i < (I32)heightmapWidth ? i : i - 1, 
                                            j < (I32)heightmapHeight ? j : j - 1, heightmapWidth);

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

                U32 idxIMG = TER_COORD<U32>(i < (I32)heightmapWidth ? i : i - 1, 
                                            j < (I32)heightmapHeight ? j : j - 1, heightmapWidth);

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
    groundVB->Create();
    terrain->buildQuadtree();

    initializeVegetation(terrain, terrainDescriptor);
    PRINT_FN(Locale::get("TERRAIN_LOAD_END"), terrain->getName().c_str());
    return true;
}

void TerrainLoader::initializeVegetation(Terrain* terrain, TerrainDescriptor* terrainDescriptor) {
    U8 textureCount = 0;
    stringImpl textureLocation;

    stringImpl currentImage = terrainDescriptor->getVariable("grassBillboard1");
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

    terrain->_vegDetails.billboardCount = textureCount;
    terrain->_vegDetails.name = terrainDescriptor->getName() + "_grass";
    terrain->_vegDetails.grassDensity = terrainDescriptor->getGrassDensity();
    terrain->_vegDetails.grassScale = terrainDescriptor->getGrassScale();
    terrain->_vegDetails.treeDensity = terrainDescriptor->getTreeDensity();
    terrain->_vegDetails.treeScale = terrainDescriptor->getTreeScale();
    terrain->_vegDetails.map = terrainDescriptor->getVariable("grassMap");
    terrain->_vegDetails.grassBillboards = grassBillboardArray;
    terrain->_vegDetails.grassShaderName = "grass";
    terrain->_vegDetails.parentTerrain = terrain;
}

bool TerrainLoader::Save(const char* fileName){
    return true;
}

bool TerrainLoader::Load(const char* filename){
    return true;
}

};