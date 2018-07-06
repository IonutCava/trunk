#include "Headers\Vegetation.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Environment/Terrain/Quadtree/Headers/Quadtree.h"
#include "Environment/Terrain/Quadtree/Headers/QuadtreeNode.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

Vegetation::Vegetation(const std::string& name, U16 billboardCount, D32 grassDensity, F32 grassScale, D32 treeDensity, F32 treeScale,
    const std::string& map, Texture* grassBillboards,
    const std::string& grassShaderName) : SceneNode(name, TYPE_VEGETATION_GRASS),
    _billboardCount(billboardCount),
    _grassDensity(grassDensity),
    _grassScale(grassScale),
    _treeScale(treeScale),
    _treeDensity(treeDensity),
    _grassBillboards(grassBillboards),
    _render(false),
    _success(false),
    _culledFinal(false),
    _shadowMapped(true),
    _threadedLoadComplete(false),
    _stopLoadingRequest(false),
    _terrain(nullptr),
    _terrainSGN(nullptr),
    _instanceCountGrass(0),
    _instanceCountTrees(0),
    _billboardDivisor(0),
    _stateRefreshIntervalBuffer(0ULL),
    _stateRefreshInterval(getSecToUs(1)) ///<Every second?
{
    _shadowQueryID = 0;
    _drawQueryID = 1;
    _map.create(map);
    _grassShaderName = grassShaderName;
    _grassShader[0] = nullptr;
    _grassShader[1] = nullptr;
    _grassShader[2] = nullptr;
    _grassGPUBuffer = GFX_DEVICE.newGVD();
    _treeGPUBuffer = GFX_DEVICE.newGVD();
    ResourceDescriptor instanceCullShader("instanceCull");
    instanceCullShader.setThreadedLoading(false);
    instanceCullShader.setId(2);
    _cullShader = CreateResource<ShaderProgram>(instanceCullShader);
}

Vegetation::~Vegetation(){
    PRINT_FN(Locale::get("UNLOAD_VEGETATION_BEGIN"),_terrain->getName().c_str());
    _stopLoadingRequest = true;
    while(!_threadedLoadComplete){
        // wait for the loading thread to finish first;
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    _grassTextureIdx.clear();
    _grassPositions.clear();
    _grassSizes.clear();
    RemoveResource(_cullShader);
    RemoveResource(_grassShader[0/*FINAL_STAGE*/]);
    RemoveResource(_grassShader[1/*SHADOW_STAGE*/]);
    RemoveResource(_grassShader[2/*Z_PRE_PASS_STAGE*/]);
    RemoveResource(_grassBillboards);

    SAFE_DELETE(_grassGPUBuffer);
    SAFE_DELETE(_treeGPUBuffer);
    PRINT_FN(Locale::get("UNLOAD_VEGETATION_END"));
}

void Vegetation::initialize(Terrain* const terrain, SceneGraphNode* const terrainSGN) {
    assert(terrain);
    assert(_map.data() != nullptr);

    ResourceDescriptor grassDepthPass(_grassShaderName + ".Depth.Shadow");
    ResourceDescriptor grassPrePass(_grassShaderName+ ".Depth.PrePass");

    _grassShader[0/*INAL_STAGE*/] = CreateResource<ShaderProgram>(ResourceDescriptor(_grassShaderName));
    _grassShader[1/*SHADOW_STAGE*/] = CreateResource<ShaderProgram>(grassDepthPass);
    _grassShader[2/*Z_PRE_PASS_STAGE*/] = CreateResource<ShaderProgram>(grassPrePass);

    _grassDensity = _grassDensity/_billboardCount;
    _terrain = terrain;
    _terrainSGN = terrainSGN;
    for (U8 i = 0; i < 3; ++i){
        _grassShader[i]->Uniform("lod_metric", 100.0f);
        _grassShader[i]->Uniform("dvd_lightCount", 1);
        _grassShader[i]->UniformTexture("texDiffuse", 0);
    }
    _cullShader->Uniform("ObjectExtent", vec3<F32>(2.0f, 2.0f, 2.0f));
    RenderStateBlockDescriptor transparent;
    transparent.setCullMode(CULL_MODE_CW);
    _grassStateBlock = GFX_DEVICE.getOrCreateStateBlock( transparent );

    Kernel* kernel = Application::getInstance().getKernel();
    _generateVegetation.reset(kernel->AddTask(0, true, true, DELEGATE_BIND(&Vegetation::generateGrass, this), DELEGATE_BIND(&Vegetation::uploadGrassData, this)));
    setState(RES_LOADED);
}

bool Vegetation::uploadGrassData(){
    if (_grassPositions.empty())
        return false;

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
            grassBlades.push_back(vertices[indices[l] + indexOffset] * _grassScale);
            texCoord.push_back(texcoords[(indices[l] + indexOffset) % 4]);
        }
    }
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
    vectorImpl<mat3<F32> > rotationMatrices;
    for (U8 i = 0; i < 18; ++i){
        temp.identity();
        temp.rotate_y(angles[i]);
        rotationMatrices.push_back(temp);
    }

    for (U8 i = 0; i < 3; ++i){
        _grassShader[i]->Uniform("positionOffsets",  grassBlades);
        _grassShader[i]->Uniform("texCoordOffsets",  texCoord);
        _grassShader[i]->Uniform("rotationMatrices", rotationMatrices);
    }

    const U32 posLocation   = Divide::VERTEX_POSITION_LOCATION;
    const U32 sizeLocation  = 11;
    const U32 texLocation   = 12;

    size_t textureSize  = std::min((I32)_billboardCount, 4) * sizeof(I32);
    size_t scaleSize    = _instanceCountGrass * sizeof(F32);
    size_t positionSize = scaleSize * 3;

    U32 billboardDiv = _billboardDivisor;
    U32 instanceDiv = 1;

    enum BufferUsage {
        TextureIDBuffer = 0,
        UnculledScaleBuffer = 1,
        UnculledPositionBuffer = 2,
        CulledPositionBuffer = 3,
        CulledSizeBuffer = 4,
        BufferUsage_PLACEHOLDER = 5
    };

    _grassGPUBuffer->Create(BufferUsage_PLACEHOLDER, 2);
    _grassGPUBuffer->SetFeedbackBuffer(CulledPositionBuffer, 0); // position culled will be generated using transform feedback using shader output 'posLocation' writing to buffer "CulledPositionBuffer"
    _grassGPUBuffer->SetFeedbackBuffer(CulledSizeBuffer,     1);

    _grassGPUBuffer->SetBuffer(TextureIDBuffer,        textureSize,  &_grassTextureIdx[0], false, false);
    _grassGPUBuffer->SetBuffer(UnculledScaleBuffer,    scaleSize,    &_grassSizes[0],      false, false);
    _grassGPUBuffer->SetBuffer(UnculledPositionBuffer, positionSize, &_grassPositions[0],  false, false);
    _grassGPUBuffer->SetBuffer(CulledSizeBuffer,       scaleSize,    NULL,                 true,  false);
    _grassGPUBuffer->SetBuffer(CulledPositionBuffer,   positionSize, NULL,                 true,  false); // "true, false" = DYNAMIC_COPY

    _grassGPUBuffer->SetAttribute(texLocation,  TextureIDBuffer,      billboardDiv, 1, false, 0, 0, SIGNED_INT);
    _grassGPUBuffer->SetAttribute(sizeLocation, CulledSizeBuffer,     instanceDiv,  1, false, 0, 0, FLOAT_32);
    _grassGPUBuffer->SetAttribute(posLocation,  CulledPositionBuffer, instanceDiv,  3, false, 0, 0, FLOAT_32);

    _grassGPUBuffer->SetFeedbackAttribute(sizeLocation, UnculledScaleBuffer, instanceDiv, 1, false, 0, 0, FLOAT_32);
    _grassGPUBuffer->SetFeedbackAttribute(posLocation,  UnculledPositionBuffer, instanceDiv, 3, false, 0, 0, FLOAT_32);

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
        _windX = sceneState.getWindDirX();
        _windZ = sceneState.getWindDirZ();
        _windS = sceneState.getWindSpeed();
        _shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows") && sgn->getReceivesShadows();
        _grassShader[0]->Uniform("dvd_enableShadowMapping", _shadowMapped);
        for (U8 i = 0; i < 3; ++i){
            _grassShader[i]->Uniform("windDirection",vec2<F32>(_windX,_windZ));
            _grassShader[i]->Uniform("windSpeed", _windS);
            _grassShader[i]->Uniform("grassScale", _grassSize);
        }
        _stateRefreshIntervalBuffer -= _stateRefreshInterval;
    }
    _stateRefreshIntervalBuffer += deltaTime;
}

void Vegetation::gpuCull(){
    bool draw = false;
    U8 currentQuery = _shadowQueryID;
    if (GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)){
        if (LightManager::getInstance().currentShadowPass() == 0){
            draw = true;
            _culledFinal = false;
        }
    }
    if (GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE)){
        draw = !_culledFinal;
        currentQuery = _drawQueryID;
        _culledFinal = true;
    }
    if (draw){
        _cullShader->bind();
        _cullShader->uploadNodeMatrices();
        GFX_DEVICE.toggleRasterization(false);
        _grassGPUBuffer->DrawInstanced(API_POINTS, _instanceCountGrass, 0, 1, currentQuery, true);
        //_treeGPUBuffer->DrawInstanced(API_POINTS, _instanceCountTrees, 0, 1, currentQuery, true);
        GFX_DEVICE.toggleRasterization(true);
    }
}

bool Vegetation::onDraw(SceneGraphNode* const sgn, const RenderStage& renderStage){
    return !(!_render || !_success || !_threadedLoadComplete);
}

void Vegetation::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    U8 currentShader = 0;
    if(GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)) 
        currentShader = 1;
    else if (GFX_DEVICE.isCurrentRenderStage(Z_PRE_PASS_STAGE))
        currentShader = 2;

    SET_STATE_BLOCK(*_grassStateBlock);

    _grassShader[currentShader]->bind();
    _grassShader[currentShader]->uploadNodeMatrices();

    _grassBillboards->Bind(0);

    _grassGPUBuffer->DrawInstanced(TRIANGLES, _grassGPUBuffer->GetFeedbackPrimitiveCount(currentShader == 1 ? _shadowQueryID : _drawQueryID), 0, 12 * 3);

}

void Vegetation::generateTrees(){
    /*
    //--> Unique position generation
    vectorImpl<vec3<F32> > positions;
    //<-- End unique position generation
    vectorImpl<FileData>& DA = GET_ACTIVE_SCENE()->getVegetationDataArray();
    if(DA.empty()){
        ERROR_FN(Locale::get("ERROR_CREATE_TREE_NO_GEOM"));
        return;
    }
    PRINT_FN(Locale::get("CREATE_TREE_START"), _treeDensity);

    for(U16 k=0; k<(U16)_treeDensity; k++) {
        I16 map_x = (I16)random((F32)_map.dimensions().width);
        I16 map_y = (I16)random((F32)_map.dimensions().height);
        I32 map_color = _map.getColor(map_x, map_y).g;

        if(map_color < 55) {
            k--;
            continue;
        }

        vec3<F32> P = _terrain->getPosition(((F32)map_x)/_map.dimensions().width, ((F32)map_y)/_map.dimensions().height);
        P.y -= 0.2f;
        if(P.y < GET_ACTIVE_SCENE()->state().getWaterLevel()){
            k--;
            continue;
        }
        bool continueLoop = true;
        for(vec3<F32>& it : positions){
            if(it.compare(P) || (it.distance(P) < 0.02f)){
                k--;
                continueLoop = false;
                break; //iterator for
            }
        }
        if(!continueLoop) continue;
        positions.push_back(P);
        QuadtreeNode* node = _terrain->getQuadtree().FindLeaf(vec2<F32>(P.x, P.z));
        assert(node);
        TerrainChunk* chunk = node->getChunk();
        assert(chunk);
        chunk->addTree(vec4<F32>(P, random(360.0f)), _treeScale, DA[(U16)(rand() % DA.size())], _terrainSGN);
    }

    positions.clear();
    PRINT_FN(Locale::get("CREATE_TREE_END"));
    DA.empty();
    return;
    */
}

void Vegetation::generateGrass(){
    PRINT_FN(Locale::get("CREATE_GRASS_BEGIN"), (U32)_grassDensity * _billboardCount);

    I32 currentCount = std::min((I32)_billboardCount, 4);
    _billboardDivisor = (U32)(_grassDensity / currentCount);

    F32 waterLevel = GET_ACTIVE_SCENE()->state().getWaterLevel() + 1.0f;
    _grassTextureIdx.reserve(currentCount);
    _grassPositions.reserve(_billboardDivisor * currentCount);
    _grassSizes.reserve(_billboardDivisor * currentCount);

    U16 mapWidth = _map.dimensions().width;
    U16 mapHeight = _map.dimensions().height;
    F32 densityFactor = (mapWidth * mapHeight) / (_grassDensity * _billboardCount);
    #pragma omp parallel for
    for (I32 index = 0; index < currentCount; ++index){
        vec3<F32> position;
        F32 textureOffset = index * random(2.0f);
        _grassTextureIdx.push_back(index);
        for (F32 width = 0.0f; width < (F32)(_map.dimensions().width) - 1 ; width += densityFactor){
            for (F32 height = 0.0f; height < (F32)(_map.dimensions().height) - 1; height += densityFactor){
                if (_stopLoadingRequest){
                    continue;
                }
                F32 x = width  + random(1.0f) + textureOffset;
                F32 y = height + random(1.0f) + textureOffset;
                CLAMP<F32>(x, 0.0f, (F32)mapWidth);
                CLAMP<F32>(y, 0.0f, (F32)mapHeight);
                F32 x_fac = x  / _map.dimensions().width;
                F32 y_fac = y / _map.dimensions().height;
                I32 map_color = _map.getColor((U16)x, (U16)y)[index];

                
                const vec3<F32>& P = _terrain->getPosition(x_fac, y_fac);
                const vec3<F32>& N = _terrain->getNormal(x_fac, y_fac);
                if (P.y < waterLevel || N.y < 0.9f || map_color < 50){
                    random(10.0f); random(100.0f); random(1000.0f);
                    continue;
                }

                position.set(P);

                F32 grassScale = ((map_color + 1) / 256.0f);
                assert(grassScale > 0.001f);
                #pragma omp critical 
                {
                    _grassPositions.push_back(position);
                    _grassSizes.push_back(grassScale);
                    _instanceCountGrass++;
                }
            }
        }
    }
    /*#pragma omp parallel for
    for (I32 index = 0; index < currentCount; index++){
        _grassTextureIdx.push_back(index);
        for (U32 k = 0; k < _billboardDivisor; k++) {
            if (_stopLoadingRequest){
                continue;
            }

            F32 x_fac = random(1.0f);
            F32 y_fac = random(1.0f);
            U16 x = static_cast<U16>(x_fac * mapWidth);
            U16 y = static_cast<U16>(x_fac * mapHeight);

            I32 map_color = _map.getColor(x, y)[index];

            const vec3<F32>& P = _terrain->getPosition(x_fac, y_fac);
            const vec3<F32>& N = _terrain->getNormal(x_fac, y_fac);

            if (P.y < waterLevel || N.y < 0.7f || map_color < 50){
                random(10.0f); random(100.0f); random(1000.0f);
                k--;
                continue;
            }
            F32 grassScale = ((map_color + 1) / 256.0f) * _grassScale;
            assert(grassScale > 0.001f);
            #pragma omp critical 
            {
                _grassPositions.push_back(P);
                _grassSizes.push_back(grassScale);
                _instanceCountGrass++;
           }
        }
    }*/

    PRINT_FN(Locale::get("CREATE_GRASS_END"));
}
