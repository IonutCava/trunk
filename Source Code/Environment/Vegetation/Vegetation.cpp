#include "Headers\Vegetation.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Environment/Terrain/Quadtree/Headers/Quadtree.h"
#include "Environment/Terrain/Quadtree/Headers/QuadtreeNode.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

bool Vegetation::_staticDataUpdated = false;

Vegetation::Vegetation(const VegetationDetails& details) : SceneNode(details.name, TYPE_VEGETATION_GRASS),
    _billboardCount(details.billboardCount),
    _grassDensity(details.grassDensity),
    _grassScale(details.grassScale),
    _treeScale(details.treeScale),
    _treeDensity(details.treeDensity),
    _grassBillboards(details.grassBillboards),
    _terrain(details.parentTerrain),
    _render(false),
    _success(false),
    _culledFinal(false),
    _shadowMapped(true),
    _threadedLoadComplete(false),
    _stopLoadingRequest(false),
    _terrainSGN(nullptr),
    _terrainChunk(nullptr),
    _instanceCountGrass(0),
    _instanceCountTrees(0),
    _grassStateBlockHash(0),
    _stateRefreshIntervalBuffer(0ULL),
    _stateRefreshInterval(getSecToUs(1)) ///<Every second?
{
    _readBuffer = 1;
    _writeBuffer = 0;

    _map.create(details.map);
    _grassShaderName = details.grassShaderName;
    _grassGPUBuffer[0] = GFX_DEVICE.newGVD(false);
    _grassGPUBuffer[1] = GFX_DEVICE.newGVD(false);
    _treeGPUBuffer[0] = GFX_DEVICE.newGVD(false);
    _treeGPUBuffer[1] = GFX_DEVICE.newGVD(false);
    _grassMatrices = GFX_DEVICE.newSB(true);

    _cullDrawCommand   = GenericDrawCommand(API_POINTS,     0, 1);
    _renderDrawCommand = GenericDrawCommand(TRIANGLE_STRIP, 0, 12 * 3);

    memset(_instanceRoutineIdx, 0, CullType_PLACEHOLDER * sizeof(U32));

    ResourceDescriptor instanceCullShader("instanceCull");
    instanceCullShader.setThreadedLoading(false);
    instanceCullShader.setId(3);
    _cullShader = CreateResource<ShaderProgram>(instanceCullShader);
}

Vegetation::~Vegetation(){
    PRINT_FN(Locale::get("UNLOAD_VEGETATION_BEGIN"), getName().c_str());
    _stopLoadingRequest = true;
    U32 timer = 0;
    while(!_threadedLoadComplete){
        // wait for the loading thread to finish first;
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        timer += 10;
        if (timer > 1500) break;
    }
    assert(_threadedLoadComplete);
    _grassPositions.clear();
    RemoveResource(_cullShader);

    SAFE_DELETE(_grassGPUBuffer[0]);
    SAFE_DELETE(_grassGPUBuffer[1]);
    SAFE_DELETE(_treeGPUBuffer[0]);
    SAFE_DELETE(_treeGPUBuffer[1]);
    SAFE_DELETE(_grassMatrices);
    PRINT_FN(Locale::get("UNLOAD_VEGETATION_END"));
}

void Vegetation::initialize(TerrainChunk* const terrainChunk, SceneGraphNode* const terrainSGN) {
    assert(terrainChunk);
    assert(_map.data() != nullptr);

    _terrainChunk = terrainChunk;
    _terrainSGN = terrainSGN;
    
    _cullShader->Uniform("ObjectExtent", vec3<F32>(1.0f, 1.0f, 1.0f));
    _cullShader->UniformTexture("HiZBuffer", 0);
    _cullShader->Uniform("dvd_frustumBias", 12.5f);
    _instanceRoutineIdx[PASS_THROUGH]             = _cullShader->GetSubroutineIndex(VERTEX_SHADER, "PassThrough");
    _instanceRoutineIdx[INSTANCE_CLOUD_REDUCTION] = _cullShader->GetSubroutineIndex(VERTEX_SHADER, "InstanceCloudReduction");
    _instanceRoutineIdx[HI_Z_CULL]                = _cullShader->GetSubroutineIndex(VERTEX_SHADER, "HiZOcclusionCull");

    RenderStateBlockDescriptor transparent;
    transparent.setCullMode(CULL_MODE_CW);
    //transparent.setBlend(true);
    _grassStateBlockHash = GFX_DEVICE.getOrCreateStateBlock( transparent );

    ResourceDescriptor vegetationMaterial("vegetationMaterial" + getName());
    Material* vegMaterial = CreateResource<Material>(vegetationMaterial);

    vegMaterial->setDiffuse(DefaultColors::WHITE());
    vegMaterial->setAmbient(DefaultColors::WHITE() / 3);
    vegMaterial->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    vegMaterial->setShininess(5.0f);
    vegMaterial->addShaderDefines("SKIP_TEXTURES");
    vegMaterial->setShaderProgram(_grassShaderName, FINAL_STAGE, true);
    vegMaterial->setShaderProgram(_grassShaderName + ".Shadow", SHADOW_STAGE, true);
    vegMaterial->setShaderProgram(_grassShaderName + ".PrePass", Z_PRE_PASS_STAGE, true);
    vegMaterial->setShaderLoadThreaded(false);
    vegMaterial->dumpToFile(false);
    setMaterial(vegMaterial);

    Kernel* kernel = Application::getInstance().getKernel();
    _generateVegetation.reset(kernel->AddTask(0, true, true, DELEGATE_BIND(&Vegetation::generateGrass, this), DELEGATE_BIND(&Vegetation::uploadGrassData, this)));
    setState(RES_LOADED);
}

namespace{
    enum BufferUsage {
        UnculledPositionBuffer = 0,
        CulledPositionBuffer = 1,
        UnculledSizeBuffer = 2,
        CulledSizeBuffer = 3,
        CulledInstanceBuffer = 4,
        BufferUsage_PLACEHOLDER = 5
    };

    const U32 posLocation   = 10;
    const U32 scaleLocation = 11;
    const U32 instLocation  = 12;
    const U32 instanceDiv = 1;
};

bool Vegetation::uploadGrassData(){
    if (_grassPositions.empty()){
        _threadedLoadComplete = true;
        return false;
    }

    static const vec2<F32> pos000(cosf(RADIANS(0.000f)), sinf(RADIANS(0.000f)));
    static const vec2<F32> pos120(cosf(RADIANS(120.0f)), sinf(RADIANS(120.0f)));
    static const vec2<F32> pos240(cosf(RADIANS(240.0f)), sinf(RADIANS(240.0f)));

    static const vec3<F32> vertices[] = {
        vec3<F32>(-pos000.x, 0.0f, -pos000.y), vec3<F32>(-pos000.x, 1.0f, -pos000.y), vec3<F32>(pos000.x, 1.0f, pos000.y), vec3<F32>(pos000.x, 0.0f, pos000.y),
        vec3<F32>(-pos120.x, 0.0f, -pos120.y), vec3<F32>(-pos120.x, 1.0f, -pos120.y), vec3<F32>(pos120.x, 1.0f, pos120.y), vec3<F32>(pos120.x, 0.0f, pos120.y),
        vec3<F32>(-pos240.x, 0.0f, -pos240.y), vec3<F32>(-pos240.x, 1.0f, -pos240.y), vec3<F32>(pos240.x, 1.0f, pos240.y), vec3<F32>(pos240.x, 0.0f, pos240.y)
    };

    static const U32 indices[] = { 2, 1, 0, 2, 0, 1, 2, 0, 3, 2, 3, 0 };

    static const vec2<F32> texcoords[] = { vec2<F32>(0.0f, 0.99f), vec2<F32>(0.0f, 0.01f), vec2<F32>(1.0f, 0.01f), vec2<F32>(1.0f, 0.99f) };

    vectorImpl<vec3<F32> > grassBlades;
    vectorImpl<vec2<F32> > texCoord;

    U32 indexOffset = 0;
    for (U8 j = 0; j < 3; ++j) {
        indexOffset = (j * 4);
        for (U8 l = 0; l < 12; ++l) {
            grassBlades.push_back(vertices[indices[l] + indexOffset]);
            texCoord.push_back(texcoords[(indices[l] + indexOffset) % 4]);
        }
    }
    
    static vectorImpl<mat3<F32> > rotationMatrices;
    if (rotationMatrices.empty()){
        vectorImpl<F32 > angles;
        angles.resize(18, 0.0f);
        for (U8 i = 0; i < 18; ++i){
            F32 temp = random(360.0f);
            while (std::find(angles.begin(), angles.end(), temp) != angles.end()){
                temp = random(360.0f);
            }
            angles[i] = temp;
        }

        mat3<F32> temp;
        for (U8 i = 0; i < 18; ++i){
            temp.identity();
            temp.rotate_y(angles[i]);
            rotationMatrices.push_back(temp);
        }
    }

    Material* mat = getMaterial();
    for (U8 i = 0; i < 3; ++i){
        ShaderProgram* const shaderProg = mat->getShaderInfo(i == 0 ? FINAL_STAGE : (i == 1 ? SHADOW_STAGE : Z_PRE_PASS_STAGE)).getProgram();
        shaderProg->Uniform("positionOffsets", grassBlades);
        shaderProg->Uniform("texCoordOffsets", texCoord);
        shaderProg->Uniform("rotationMatrices", rotationMatrices);
        shaderProg->Uniform("lod_metric", 100.0f);
        shaderProg->UniformTexture("texDiffuseGrass", 0);
    }
    
    for(U8 i = 0; i < 2; ++i){
        GenericVertexData* buffer = _grassGPUBuffer[i];

        buffer->Create(BufferUsage_PLACEHOLDER, 3);
        buffer->SetFeedbackBuffer(CulledPositionBuffer, 0); // position culled will be generated using transform feedback using shader output 'posLocation' writing to buffer "CulledPositionBuffer"
        buffer->SetFeedbackBuffer(CulledSizeBuffer,     1);
        buffer->SetFeedbackBuffer(CulledInstanceBuffer, 2);

        buffer->SetBuffer(UnculledPositionBuffer, _instanceCountGrass,     sizeof(vec4<F32>), &_grassPositions[0], false, false);
        buffer->SetBuffer(UnculledSizeBuffer,     _instanceCountGrass,     sizeof(F32),       &_grassScales[0],    false, false);
        buffer->SetBuffer(CulledPositionBuffer,   _instanceCountGrass * 3, sizeof(vec4<F32>), NULL,                true,  false); // "true, false" = DYNAMIC_COPY
        buffer->SetBuffer(CulledSizeBuffer,       _instanceCountGrass * 3, sizeof(F32),       NULL,                true,  false);
        buffer->SetBuffer(CulledInstanceBuffer,   _instanceCountGrass * 3, sizeof(I32),       NULL,                true,  false);

        buffer->getDrawAttribDescriptor(posLocation).set(CulledPositionBuffer,   instanceDiv, 4, false, 0, 0, FLOAT_32);
        buffer->getDrawAttribDescriptor(scaleLocation).set(CulledSizeBuffer,     instanceDiv, 1, false, 0, 0, FLOAT_32);
        buffer->getDrawAttribDescriptor(instLocation).set( CulledInstanceBuffer, instanceDiv, 1, false, 0, 0, SIGNED_INT);
        buffer->getFdbkAttribDescriptor(posLocation).set(UnculledPositionBuffer, instanceDiv, 4, false, 0, 0, FLOAT_32);
        buffer->getFdbkAttribDescriptor(scaleLocation).set(UnculledSizeBuffer,   instanceDiv, 1, false, 0, 0, FLOAT_32);

        /*
        _grassMatrices->Create(false, false, (I32)_grassMatricesTemp.size(), sizeof(_grassMatricesTemp[0]));
        _grassMatrices->UpdateData(0, _grassMatricesTemp.size() * sizeof(_grassMatricesTemp[0]), &_grassMatricesTemp[0]);
        _grassMatrices->bind(10);
        _grassMatricesTemp.clear();
        */

        buffer->toggleDoubleBufferedQueries(false);
    }

    _grassPositions.clear();
    _grassScales.clear();

    _render = _threadedLoadComplete = true;

    return true;
}

void Vegetation::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    if(_threadedLoadComplete && !_success){
        generateTrees();
        sceneState.getRenderState().getCameraMgr().addCameraUpdateListener(DELEGATE_BIND(&Vegetation::gpuCull, this));
        _success = true;
    }
      
    if(!_render || !_success) return;

    //Query shadow state every "_stateRefreshInterval" microseconds
    if (_stateRefreshIntervalBuffer >= _stateRefreshInterval){
        if(!_staticDataUpdated){
            _windX = sceneState.getWindDirX();
            _windZ = sceneState.getWindDirZ();
            _windS = sceneState.getWindSpeed();
            Material* mat = getMaterial();
            for (U8 i = 0; i < 3; ++i){
                RenderStage stage = (i == 0 ? FINAL_STAGE : (i == 1 ? SHADOW_STAGE : Z_PRE_PASS_STAGE));
                mat->getShaderInfo(stage).getProgram()->Uniform("grassScale",/* _grassSize*/1.0f);
            }
             _stateRefreshIntervalBuffer -= _stateRefreshInterval;
            _cullShader->Uniform("dvd_visibilityDistance", sceneState.getGrassVisibility());
            _staticDataUpdated = true;
        }
    }
    _stateRefreshIntervalBuffer += deltaTime;

    _writeBuffer = (_writeBuffer + 1) % 2;
    _readBuffer = (_writeBuffer + 1) % 2;
    _culledFinal = false;
}

U32 Vegetation::getQueryID(){
    switch (GFX_DEVICE.getRenderStage()){
        case SHADOW_STAGE: return 0;
        case REFLECTION_STAGE: return 1;
        default: return 2;
    };
}

void Vegetation::gpuCull(){
    U32 queryId = getQueryID();

    if (GFX_DEVICE.is2DRendering()) return;

    bool draw = false;
    switch (queryId){
        case 0/*SHADOW_STAGE*/: {
            draw = LightManager::getInstance().currentShadowPass() == 0;
            _culledFinal = false;
        }break;
        case 1/*REFLECTION_STAGE*/:{
            draw = true;
            _culledFinal = false;
        }break;
        default:{
            draw = !_culledFinal;
            _culledFinal = true;
        }break;
    }

    if (draw && _threadedLoadComplete && _terrainChunk->getLoD() == 0){
        GenericVertexData* buffer = _grassGPUBuffer[_writeBuffer];
        _cullShader->bind();
        //_cullShader->SetSubroutine(VERTEX_SHADER, _instanceRoutineIdx[HI_Z_CULL]);
        _cullShader->Uniform("cullType", /*queryId*/(U32)INSTANCE_CLOUD_REDUCTION);
        _cullShader->uploadNodeMatrices();
        buffer->setShaderProgram(_cullShader);

        GFX_DEVICE.toggleRasterization(false);
        GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Bind(0, TextureDescriptor::Depth);
        buffer->BindFeedbackBufferRange(CulledPositionBuffer, _instanceCountGrass * queryId, _instanceCountGrass);
        buffer->BindFeedbackBufferRange(CulledSizeBuffer,     _instanceCountGrass * queryId, _instanceCountGrass);
        buffer->BindFeedbackBufferRange(CulledInstanceBuffer, _instanceCountGrass * queryId, _instanceCountGrass);

        _cullDrawCommand.setInstanceCount(_instanceCountGrass);
        _cullDrawCommand.setQueryID(queryId);
        _cullDrawCommand.setDrawToBuffer(true);
        buffer->Draw(_cullDrawCommand);
        //_cullDrawCommand.setInstanceCount(_instanceCountTrees);
        //_treeGPUBuffer->Draw(_cullDrawCommand);
        GFX_DEVICE.toggleRasterization(true);
    }
}

bool Vegetation::onDraw(SceneGraphNode* const sgn, const RenderStage& renderStage){
    _staticDataUpdated = false;
    return !(!_render || !_success || !_threadedLoadComplete || _terrainChunk->getLoD() > 0 || (LightManager::getInstance().currentShadowPass() > 0 && GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)));
}

bool Vegetation::setMaterialInternal(SceneGraphNode* const sgn) {
    bool depthPass = GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE);
    bool depthPrePass = GFX_DEVICE.isDepthPrePass();

    LightManager& lightMgr = LightManager::getInstance();
    Material::ShaderInfo& shaderInfo = getMaterial()->getShaderInfo(depthPass ? (depthPrePass ? Z_PRE_PASS_STAGE : SHADOW_STAGE) : FINAL_STAGE);
    _drawShader = shaderInfo.getProgram();
    if (!depthPass){
        getMaterial()->UploadToShader(shaderInfo);
        _drawShader->Uniform("dvd_enableShadowMapping", lightMgr.shadowMappingEnabled() && sgn->getReceivesShadows());
    }
    
    return _drawShader->bind();
}

void Vegetation::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    U32 queryId = getQueryID();
    //gpuCull();
    GenericVertexData* buffer = _grassGPUBuffer[_readBuffer];
    I32 instanceCount = buffer->GetFeedbackPrimitiveCount(queryId);
    if (instanceCount == 0)
        return;

    _grassBillboards->Bind(0);
    if(setMaterialInternal(sgn)){
        _renderDrawCommand.setStateHash(_grassStateBlockHash);
        _renderDrawCommand.setInstanceCount(instanceCount);
        _renderDrawCommand.setLoD(1);

        SET_STATE_BLOCK(_grassStateBlockHash, true);
        buffer->setShaderProgram(_drawShader);
        buffer->getDrawAttribDescriptor(posLocation).setOffset(_instanceCountGrass * queryId);
        buffer->getDrawAttribDescriptor(scaleLocation).setOffset(_instanceCountGrass * queryId);
        buffer->getDrawAttribDescriptor(instLocation).setOffset(_instanceCountGrass * queryId);
        buffer->Draw(_renderDrawCommand);
    }
}

void Vegetation::generateTrees(){

}

void Vegetation::generateGrass(){
    const vec2<F32>& chunkPos  = _terrainChunk->getOffsetAndSize().xy();
    const vec2<F32>& chunkSize = _terrainChunk->getOffsetAndSize().zw();
    const F32 waterLevel = GET_ACTIVE_SCENE()->state().getWaterLevel() + 1.0f;
    const I32 currentCount = std::min((I32)_billboardCount, 4);
    const U16 mapWidth = _map.dimensions().width;
    const U16 mapHeight = _map.dimensions().height;
    const U32 grassElements = _grassDensity * chunkSize.x * chunkSize.y;

    PRINT_FN(Locale::get("CREATE_GRASS_BEGIN"), grassElements);

    _grassPositions.reserve(grassElements);
    //_grassMatricesTemp.reserve(grassElements);
    F32 densityFactor = 1.0f / _grassDensity;
    #pragma omp parallel for
    for (I32 index = 0; index < currentCount; ++index){
        densityFactor += 0.1f;
        for (F32 width = 0; width < chunkSize.x - densityFactor; width += densityFactor){
            for (F32 height = 0; height < chunkSize.y - densityFactor; height += densityFactor){
                if (_stopLoadingRequest)
                    continue;
                
                F32 x = width  + random(densityFactor) + chunkPos.x;
                F32 y = height + random(densityFactor) + chunkPos.y;
                CLAMP<F32>(x, 0.0f, (F32)mapWidth  - 1.0f);
                CLAMP<F32>(y, 0.0f, (F32)mapHeight - 1.0f);
                F32 x_fac = x  / _map.dimensions().width;
                F32 y_fac = y  / _map.dimensions().height;

                I32 map_color = _map.getColor((U16)x, (U16)y)[index];
                if (map_color < 150) continue;

                const vec3<F32>& P = _terrain->getPosition(x_fac, y_fac);
                if (P.y < waterLevel) continue;

                const vec3<F32>& N = _terrain->getNormal(x_fac, y_fac);
                if (N.y < 0.7f) continue;
                    
                #pragma omp critical 
                {
                    /*position.set(P);
                    mat4<F32> matRot1;
                    matRot1.scale(vec3<F32>(grassScale));
                    mat4<F32> matRot2;
                    matRot2.rotate_y(random(360.0f));
                    _grassMatricesTemp.push_back(matRot1 * matRot2 * rotationFromVToU(WORLD_Y_AXIS, N).getMatrix());*/

                    _grassPositions.push_back(vec4<F32>(P, index));
                    _grassScales.push_back(((map_color + 1) / 256.0f));
                    _instanceCountGrass++;
                }
            }
        }
    }

    PRINT_FN(Locale::get("CREATE_GRASS_END"));
}
