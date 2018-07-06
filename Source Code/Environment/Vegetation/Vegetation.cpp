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
	SAFE_DELETE(_grassVBO);

	RemoveResource(_grassShader);
	for(U8 i = 0; i < _grassBillboards.size(); i++){
		RemoveResource(_grassBillboards[i]);
	}

	SAFE_DELETE(_grassStateBlock);

	PRINT_FN(Locale::get("UNLOAD_VEGETATION_END"));
}

void Vegetation::initialize(const std::string& grassShader, Terrain* const terrain, SceneGraphNode* const terrainSGN) {
	assert(terrain);
	_grassShader  = CreateResource<ShaderProgram>(ResourceDescriptor(grassShader));
	_grassDensity = _grassDensity/_billboardCount;
	_terrain = terrain;
	_terrainSGN = terrainSGN;
    _grassVBO = GFX_DEVICE.newVBO(QUADS);
	_grassVBO->useHWIndices(false);//<Use custom LOD indices;
	_grassVBO->useLargeIndices(true);
	_grassVBO->computeTriangles(false);
    U32 size = (U32) _grassDensity * _billboardCount * 12;
	_grassVBO->reservePositionCount( size );
	_grassVBO->reserveNormalCount( size );
	_grassVBO->getTexcoord().reserve( size );
    _grassVBOBillboardIndice.resize(_billboardCount,0);

    assert(_map.data);
	_success = generateGrass(_billboardCount,size);
   
	_grassShader->Uniform("lod_metric", 100.0f);
    _grassShader->Uniform("dvd_lightCount", 1);
	_grassShader->Uniform("grassScale", _grassScale);
	_grassShader->UniformTexture("texDiffuse", 0);
	
	_grassVBO->setShaderProgram(_grassShader);
	if(_success) _success = generateTrees();

	RenderStateBlockDescriptor transparent;
    transparent.setCullMode(CULL_MODE_NONE);
    transparent.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
    _grassStateBlock = GFX_DEVICE.createStateBlock( transparent );

	_render = true;
}

void Vegetation::sceneUpdate(const U32 sceneTime,SceneGraphNode* const sgn){
	if(!_render || !_success) return;
	///Query shadow state every "_stateRefreshInterval" milliseconds
	if (sceneTime - _stateRefreshIntervalBuffer >= _stateRefreshInterval){
		Scene* activeScene = GET_ACTIVE_SCENE();
		_windX = activeScene->state()->getWindDirX();
		_windZ = activeScene->state()->getWindDirZ();
		_windS = activeScene->state()->getWindSpeed();
		_shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");
        _grassShader->bind();
        _grassShader->Uniform("windDirection",vec2<F32>(_windX,_windZ));
	    _grassShader->Uniform("windSpeed", _windS);
    	_grassShader->Uniform("dvd_enableShadowMapping", _shadowMapped);
        _grassShader->Uniform("worldHalfExtent", GET_ACTIVE_SCENE()->getSceneGraph()->getRoot()->getBoundingBox().getWidth() * 0.5f);
        _grassShader->unbind();
		_stateRefreshIntervalBuffer += _stateRefreshInterval;
	}
}

void Vegetation::draw(const RenderStage& currentStage, Transform* const parentTransform){
	if(!_render || !_success) return;
	if(GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)) return;

     SET_STATE_BLOCK(_grassStateBlock);

	_grassShader->bind();

	_grassShader->uploadModelMatrices();

	for(U32 index = 0; index < _billboardCount; index++){
        _grassBillboards[index]->Bind(0);

        _terrain->getQuadtree().DrawGrass(index,parentTransform);

        _grassBillboards[index]->Unbind(0);
    }
}

bool Vegetation::generateGrass(U32 billboardCount, U32 size){
	PRINT_FN(Locale::get("CREATE_GRASS_BEGIN"), (U32)_grassDensity * billboardCount);
	vec2<F32> pos0(cosf(RADIANS(0.0f)), sinf(RADIANS(0.0f)));
	vec2<F32> pos120(cosf(RADIANS(120.0f)), sinf(RADIANS(120.0f)));
	vec2<F32> pos240(cosf(RADIANS(240.0f)), sinf(RADIANS(240.0f)));

	vec3<F32> tVertices[] = {
		vec3<F32>(-pos0.x,   -pos0.y,   0.0f),	vec3<F32>(-pos0.x,   -pos0.y,   1.0f),	vec3<F32>(pos0.x,   pos0.y,   1.0f),	vec3<F32>(pos0.x,   pos0.y,   0.0f),
		vec3<F32>(-pos120.x, -pos120.y, 0.0f),	vec3<F32>(-pos120.x, -pos120.y, 1.0f),	vec3<F32>(pos120.x, pos120.y, 1.0f),	vec3<F32>(pos120.x, pos120.y, 0.0f),
		vec3<F32>(-pos240.x, -pos240.y, 0.0f),	vec3<F32>(-pos240.x, -pos240.y, 1.0f),	vec3<F32>(pos240.x, pos240.y, 1.0f),	vec3<F32>(pos240.x, pos240.y, 0.0f)
	};
	vec2<F32> tTexcoords[] = {
		vec2<F32>(0.0f, 0.49f), vec2<F32>(0.0f, 0.01f), vec2<F32>(1.0f, 0.01f), vec2<F32>(1.0f, 0.49f),
		vec2<F32>(0.0f, 0.49f), vec2<F32>(0.0f, 0.01f), vec2<F32>(1.0f, 0.01f), vec2<F32>(1.0f, 0.49f),
		vec2<F32>(0.0f, 0.49f), vec2<F32>(0.0f, 0.01f), vec2<F32>(1.0f, 0.01f), vec2<F32>(1.0f, 0.49f)
	};

    for(U8 index = 0; index < billboardCount; index++){
        _grassVBOBillboardIndice[index] = _grassVBO->getPosition().size();

		mat3<F32> matRot;
		vec2<F32> uv_offset(0,0);
		vec3<I32> map_color;
		vec3<F32> data;
		vec3<F32> currentVertex;
		F32 grassScaleFactor = 256/_grassScale;

        for(U32 k=0; k<(U32)_grassDensity; k++) {
		    F32 x = random(1.0f);
		    F32 y = random(1.0f);

			const vec3<F32>& P = _terrain->getPosition(x, y);
		    if(P.y < GET_ACTIVE_SCENE()->state()->getWaterLevel()){
			    k--;
			    continue;
		    }

		    uv_offset.y = random(3)==0 ? 0.0f : 0.5f;
		    map_color.set(_map.getColor((U16)(x * _map.w), (U16)(y * _map.h)));

		    if(map_color.g < 150) {
    			k--;
	    		continue;
		    }

		    _grassSize = (F32)(map_color.g+1) / grassScaleFactor;

		    const vec3<F32>& N = _terrain->getNormal(x, y);
		    if(N.y < 0.8f) {
			    k--;
			    continue;
		    }

			matRot.identity();
			matRot.rotate_z(random(360.0f));

			QuadtreeNode* node = _terrain->getQuadtree().FindLeaf(vec2<F32>(P.x, P.z));
			assert(node);
			TerrainChunk* chunk = node->getChunk();
			assert(chunk);
            ChunkGrassData& chunkGrassData = chunk->getGrassData();

            if(chunkGrassData.empty()){
                 chunkGrassData._grassIndice.resize(_billboardCount);
                 chunkGrassData._grassVBO = _grassVBO;
            }

            U32 idx = (U32)_grassVBOBillboardIndice[index] + chunkGrassData._grassIndice[index].size();

			const vec3<F32>& T = _terrain->getTangent(x, y);
		    const vec3<F32>& B = _terrain->getBiTangent(x, y);

    		for(U8 i=0; i < 12 /*3 * 4 */; i++)	{
    			data.set(matRot*(tVertices[i]*_grassSize));
	    		currentVertex.set(P);
		    	currentVertex.x += Dot(data, T);
			   	currentVertex.y += Dot(data, B) - 0.075f;
			    currentVertex.z += Dot(data, N);

			    _grassVBO->addPosition(currentVertex);
			    _grassVBO->addNormal( tTexcoords[i].t < 0.2f ? N : -N );
			    _grassVBO->getTexcoord().push_back( uv_offset + tTexcoords[i] );
				_grassVBO->addIndex(idx+i);
			    chunkGrassData._grassIndice[index].push_back(idx+i);
 			}
        }
	}
    _grassVBO->Create();
	PRINT_FN(Locale::get("CREATE_GRASS_END"));
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
		I16 map_x = (I16)random((F32)_map.w);
		I16 map_y = (I16)random((F32)_map.h);
		vec3<I32> map_color = _map.getColor(map_x, map_y);
		if(map_color.g < 55) {
			k--;
			continue;
		}

		vec3<F32> P = _terrain->getPosition(((F32)map_x)/_map.w, ((F32)map_y)/_map.h);
		P.y -= 0.2f;
		if(P.y < GET_ACTIVE_SCENE()->state()->getWaterLevel()){
			k--;
			continue;
		}
		bool continueLoop = true;
		for_each(vec3<F32>& it, positions){
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

		U16 index = rand() % DA.size();
		chunk->addTree(vec4<F32>(P, random(360.0f)),_treeScale,DA[index],_terrainSGN);
	}

	positions.clear();
	PRINT_FN(Locale::get("CREATE_TREE_END"));
	DA.empty();
	return true;
}