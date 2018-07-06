#include "Headers\Vegetation.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Environment/Terrain/Quadtree/Headers/Quadtree.h"
#include "Environment/Terrain/Quadtree/Headers/QuadtreeNode.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

Vegetation::~Vegetation(){
    PRINT_FN(Locale::get("UNLOAD_VEGETATION_BEGIN"),_terrain->getName().c_str());
    _stopLoadingRequest = true;
    while(!_threadedLoadComplete){
        // wait for the loading thread to finish first;
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    RemoveResource(_grassShader);
    for(U8 i = 0; i < _grassBillboards.size(); i++){
        RemoveResource(_grassBillboards[i]);
    }

    SAFE_DELETE(_grassStateBlock);

    PRINT_FN(Locale::get("UNLOAD_VEGETATION_END"));
}

void Vegetation::initialize(const std::string& grassShader, Terrain* const terrain, SceneGraphNode* const terrainSGN) {
    assert(terrain);
    assert(_map.data() != nullptr);

    U32 size = (U32) _grassDensity * _billboardCount * 12;
    _grassShader  = CreateResource<ShaderProgram>(ResourceDescriptor(grassShader));
    _grassDensity = _grassDensity/_billboardCount;
    _terrain = terrain;
    _terrainSGN = terrainSGN;
    _grassShader->Uniform("lod_metric", 100.0f);
    _grassShader->Uniform("dvd_lightCount", 1);
    _grassShader->UniformTexture("texDiffuse", 0);

    RenderStateBlockDescriptor transparent;
    transparent.setCullMode(CULL_MODE_CW);
    transparent.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
    _grassStateBlock = GFX_DEVICE.createStateBlock( transparent );

    Kernel* kernel = Application::getInstance().getKernel();
    New Task(kernel->getThreadPool(), 0, true, true, DELEGATE_BIND(&Vegetation::generateGrass, this, _billboardCount, size));
}

void Vegetation::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){

    if(_threadedLoadComplete && !_success){
        _success = generateTrees();
        if(_success) _render = true;
    }
      
    if(!_render || !_success) return;

    //Query shadow state every "_stateRefreshInterval" microseconds
    if (_stateRefreshIntervalBuffer >= _stateRefreshInterval){
        _windX = sceneState.getWindDirX();
        _windZ = sceneState.getWindDirZ();
        _windS = sceneState.getWindSpeed();
        _shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows") && sgn->getReceivesShadows();
        _grassShader->Uniform("windDirection",vec2<F32>(_windX,_windZ));
        _grassShader->Uniform("windSpeed", _windS);
        _grassShader->Uniform("grassScale", _grassSize);
        _grassShader->Uniform("dvd_enableShadowMapping", _shadowMapped);
        _grassShader->Uniform("worldHalfExtent", LightManager::getInstance().getLigthOrthoHalfExtent());
        _stateRefreshIntervalBuffer -= _stateRefreshInterval;
    }
    _stateRefreshIntervalBuffer += deltaTime;
}

void Vegetation::render(SceneGraphNode* const sgn){
    if(!_render || !_success || !_threadedLoadComplete) return;
    if(GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)) return;

     SET_STATE_BLOCK(_grassStateBlock);

    _grassShader->bind();
    _grassShader->UniformTexture("texDiffuse", 0);

    for(U32 index = 0; index < _billboardCount; index++){
        _grassBillboards[index]->Bind(0);

        _terrain->getQuadtree().DrawGrass(index, nullptr);

        _grassBillboards[index]->Unbind(0);
    }
}

bool Vegetation::generateGrass(U32 billboardCount, U32 size){
    PRINT_FN(Locale::get("CREATE_GRASS_BEGIN"), (U32)_grassDensity * billboardCount);

    static const vec2<F32> pos000 = vec2<F32>(cosf(RADIANS(0.000f)), sinf(RADIANS(0.000f)));
    static const vec2<F32> pos120 = vec2<F32>(cosf(RADIANS(120.0f)), sinf(RADIANS(120.0f)));
    static const vec2<F32> pos240 = vec2<F32>(cosf(RADIANS(240.0f)), sinf(RADIANS(240.0f)));

    static const vec3<F32> _vertices[] = {
        vec3<F32>(-pos000.x, 0.0f, -pos000.y),	vec3<F32>(-pos000.x, 1.0f, -pos000.y),	vec3<F32>(pos000.x, 1.0f, pos000.y),	vec3<F32>(pos000.x, 0.0f, pos000.y),
        vec3<F32>(-pos120.x, 0.0f, -pos120.y),	vec3<F32>(-pos120.x, 1.0f, -pos120.y),	vec3<F32>(pos120.x, 1.0f, pos120.y),	vec3<F32>(pos120.x, 0.0f, pos120.y),
        vec3<F32>(-pos240.x, 0.0f, -pos240.y),	vec3<F32>(-pos240.x, 1.0f, -pos240.y),	vec3<F32>(pos240.x, 1.0f, pos240.y),	vec3<F32>(pos240.x, 0.0f, pos240.y)
    };

    static const vec2<F32> _texcoords[] = {
        vec2<F32>(0.0f, 0.49f), vec2<F32>(0.0f, 0.01f), vec2<F32>(1.0f, 0.01f), vec2<F32>(1.0f, 0.49f)
    };

    static const U32 indices[] = {2, 1, 0, 2, 0, 1, 2, 0, 3, 2, 3, 0};
                    
    for(U8 index = 0; index < billboardCount; index++){
        mat3<F32> matRot, matScale;
        F32 uv_offset_t = 0.0f;
        vec3<F32> data;
        vec3<F32> currentVertex;
        I32 map_color;
        // Preserve the same density when scaling grass
        _grassDensity *= (1.0 / _grassScale); 
        CLAMP<D32>(_grassDensity, 0.0, ((D32)Config::MAX_GRASS_BATCHES / 12));
        for(U32 k=0; k<(U32)_grassDensity; k++) {
            if(_stopLoadingRequest){
                _threadedLoadComplete = true;
                return false;
            }
            F32 x = random(1.0f);
            F32 y = random(1.0f);

            uv_offset_t = random(2) == 0 ? 0.0f : 0.5f;
            map_color = _map.getColor((U16)(x * _map.dimensions().width), (U16)(y * _map.dimensions().height)).g;

            const vec3<F32>& P = _terrain->getPosition(x, y);
            const vec3<F32>& T = _terrain->getTangent(x, y);
            const vec3<F32>& N = _terrain->getNormal(x, y);

            if(P.y < GET_ACTIVE_SCENE()->state().getWaterLevel() || N.y < 0.8f || map_color < 150){
                k--;
                continue;
            }

            matRot.identity();
            matRot.rotate_y(random(360.0f));
            // Negate the binormal as we are using -Z looking direction
            vec3<F32>  B = Cross(N, T) * -1.0f;

            QuadtreeNode* node = _terrain->getQuadtree().FindLeaf(vec2<F32>(P.x, P.z)); assert(node);
            TerrainChunk* chunk = node->getChunk();                                     assert(chunk);
            ChunkGrassData& chunkGrassData = chunk->getGrassData();

            if(chunkGrassData.empty()){
                assert(chunkGrassData._grassVBO == nullptr);

                U32 chunkSize = size / _terrain->getQuadtree().getChunkCount();
                chunkGrassData._grassIndices.resize(_billboardCount);
                chunkGrassData._grassIndexOffset.resize(_billboardCount);
                chunkGrassData._grassIndexSize.resize(_billboardCount);
                chunkGrassData._grassVBO = GFX_DEVICE.newVBO(TRIANGLES);
                chunkGrassData._grassVBO->useLargeIndices(true);
                chunkGrassData._grassVBO->computeTriangles(false);
                chunkGrassData._grassVBO->reservePositionCount( chunkSize);
                chunkGrassData._grassVBO->reserveNormalCount( chunkSize );
                chunkGrassData._grassVBO->getTexcoord().reserve( chunkSize );
                chunkGrassData._grassVBO->setShaderProgram(_grassShader);
            }
    
            _grassSize = ((F32)(map_color+1) / 256) * _grassScale;	
            currentVertex.set(P);
            for(U8 i=0; i < 12 ; ++i)	{ // 4 vertices per quad, 3 quads per grass element
                data.set(matRot * (_vertices[i] * _grassSize));

                chunkGrassData._grassVBO->addPosition(currentVertex + vec3<F32>(Dot(data, T), Dot(data, N), Dot(data, B)));
                chunkGrassData._grassVBO->addNormal( _texcoords[i%4].t < 0.2f ? N : -N );
                chunkGrassData._grassVBO->getTexcoord().push_back(vec2<F32>(_texcoords[i%4].s, uv_offset_t + _texcoords[i%4].t));
            }

            U32 idx = (U32)chunkGrassData._grassIndices[index].size();	
            U32 indexOffset = 0;
            for(U8 j = 0; j < 3; ++j) {
                indexOffset = idx + (j * 4);
                for(U8 l = 0; l < 12; ++l) {
                    chunkGrassData._grassIndices[index].push_back(indices[l] + indexOffset);
                }
            }
        }
    }

    _terrain->getQuadtree().GenerateGrassIndexBuffer(_billboardCount);
    
    PRINT_FN(Locale::get("CREATE_GRASS_END"));
    _threadedLoadComplete = true;
    return true;
}

bool Vegetation::generateTrees(){
    //--> Unique position generation
    vectorImpl<vec3<F32> > positions;
    //<-- End unique position generation
    vectorImpl<FileData>& DA = GET_ACTIVE_SCENE()->getVegetationDataArray();
    if(DA.empty()){
        ERROR_FN(Locale::get("ERROR_CREATE_TREE_NO_GEOM"));
        return true;
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
        FOR_EACH(vec3<F32>& it, positions){
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
    return true;
}