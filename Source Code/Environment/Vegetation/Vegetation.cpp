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
	for(U8 i = 0; i < _grassVBO.size(); i++){
		SAFE_DELETE(_grassVBO[i]);
	}
	_grassVBO.clear();
	RemoveResource(_grassShader);
	for(U8 i = 0; i < _grassBillboards.size(); i++){
		RemoveResource(_grassBillboards[i]);
	}

	SAFE_DELETE(_grassStateBlock);

	PRINT_FN(Locale::get("UNLOAD_VEGETATION_END"));
}

void Vegetation::initialize(const std::string& grassShader, Terrain* const terrain,SceneGraphNode* const terrainSGN)
{
	_grassShader  = CreateResource<ShaderProgram>(ResourceDescriptor(grassShader));
	_grassDensity = _grassDensity/_billboardCount;
	_terrain = terrain;
	_terrainSGN = terrainSGN;
	assert(_terrain);
	for(U8 i = 0 ; i < _billboardCount; i++) _success = generateGrass(i);
	if(_success) _success = generateTrees();

	RenderStateBlockDescriptor transparent;
    transparent.setCullMode(CULL_MODE_NONE);
	transparent.setAlphaTest(true);
    transparent.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
    _grassStateBlock = GFX_DEVICE.createStateBlock( transparent );

	_render = true;

}

void Vegetation::sceneUpdate(D32 sceneTime){
	if(!_render || !_success) return;
	///Query shadow state every "_stateRefreshInterval" milliseconds
	if (sceneTime - _stateRefreshIntervalBuffer >= _stateRefreshInterval){
		Scene* activeScene = GET_ACTIVE_SCENE();
		_windX = activeScene->state()->getWindDirX();
		_windZ = activeScene->state()->getWindDirZ();
		_windS = activeScene->state()->getWindSpeed();
		_shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");
		_stateRefreshIntervalBuffer += _stateRefreshInterval;
	}
}

void Vegetation::draw(bool drawInReflection){
	if(!_render || !_success) return;
	if(GFX_DEVICE.getRenderStage() == SHADOW_STAGE) return;



     SET_STATE_BLOCK(_grassStateBlock);

	_grassShader->bind();
		_grassShader->Uniform("windDirection",vec2<F32>(_windX,_windZ));
		_grassShader->Uniform("windSpeed", _windS);
		_grassShader->Uniform("light_count", 1);
		_grassShader->Uniform("enable_shadow_mapping", _shadowMapped);
		for(U16 index = 0; index < _billboardCount; index++){
			_grassBillboards[index]->Bind(0);
				_grassShader->UniformTexture("texDiffuse", 0);
		
					_grassVBO[index]->Enable();
						_terrain->getQuadtree().DrawGrass();
					_grassVBO[index]->Disable();

			_grassBillboards[index]->Unbind(0);
		}
}



bool Vegetation::generateGrass(U32 index){
	PRINT_FN(Locale::get("CREATE_GRASS_BEGIN"), (U32)_grassDensity);
	assert(_map.data);
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

	_grassVBO.push_back(GFX_DEVICE.newVBO());
	U32 size = (U32) ceil(_grassDensity) * 3 * 4;
	_grassVBO[index]->reservePositionCount( size );
	_grassVBO[index]->getNormal().reserve( size );
	_grassVBO[index]->getTexcoord().reserve( size );
	for(U32 k=0; k<(U32)_grassDensity; k++) {
		F32 x = random(1.0f);
		F32 y = random(1.0f);
		U16 map_x = (U16)(x * _map.w);
		U16 map_y = (U16)(y * _map.h);
		vec2<F32> uv_offset = vec2<F32>(0.0f, random(3)==0 ? 0.0f : 0.5f);
		F32 size = random(0.5f);
		vec3<I32> map_color = _map.getColor(map_x, map_y);
		if(map_color.g < 150) {
			k--;
			continue;
		}

		_grassSize = (F32)(map_color.g+1) / (256 / _grassScale);
		vec3<F32> P = _terrain->getPosition(x, y);
		P.y -= 0.075f;
		if(P.y < GET_ACTIVE_SCENE()->state()->getWaterLevel()){
			k--;
			continue;
		}

		vec3<F32> N = _terrain->getNormal(x, y);
		vec3<F32> T = _terrain->getTangent(x, y);
		vec3<F32> B = Cross(N, T);
	
		if(N.y < 0.8f) {
			k--;
			continue;
		} else {
			mat3<F32> matRot;
			matRot.rotate_z(random(360.0f));

			U32 idx = (U32)_grassVBO[index]->getPosition().size();

			QuadtreeNode* node = _terrain->getQuadtree().FindLeaf(vec2<F32>(P.x, P.z));
			assert(node);
			TerrainChunk* chunk = node->getChunk();
			assert(chunk);

			for(U8 i=0; i<3*4; i++)	{

				vec3<F32> data = matRot*(tVertices[i]*_grassSize);
				vec3<F32> vertex = P;
				vertex.x += Dot(data, T);
				vertex.y += Dot(data, B);
				vertex.z += Dot(data, N);
				
				_grassVBO[index]->addPosition(vertex );
				_grassVBO[index]->getNormal().push_back( tTexcoords[i].t < 0.2f ? -N : N );
				_grassVBO[index]->getTexcoord().push_back( uv_offset + tTexcoords[i] );
				chunk->getGrassIndiceArray().push_back(idx+i);
			}

		}
	}

	bool ret = _grassVBO[index]->Create();

	_grassShader->bind();
		_grassShader->Uniform("lod_metric", 100.0f);
		_grassShader->Uniform("grassScale", _grassSize);
	_grassShader->unbind();

	PRINT_FN(Locale::get("CREATE_GRASS_END"));
	return ret;
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

